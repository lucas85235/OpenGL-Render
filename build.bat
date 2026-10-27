@echo off
setlocal EnableDelayedExpansion

REM ==========================================
REM Configuracoes
REM ==========================================
set "BUILD_DIR=build"
set "BUILD_TYPE=Release"

cls
echo === Iniciando Build System (CMake + Ninja) ===

REM ==========================================
REM 1. Setup do ambiente Visual Studio (cl.exe + ninja)
REM ==========================================
if defined VSINSTALLDIR goto vs_ready

echo [*] Configurando ambiente Visual Studio...
set "VSWHERE_DIR=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%VSWHERE_DIR%\vswhere.exe" goto no_vswhere

set "VS_PATH="
pushd "%VSWHERE_DIR%"
REM Use .\ prefix because modern cmd does not search CWD by default.
REM List installs (VS 2019 through VS 2022 range) sorted newest-first.
.\vswhere.exe -products * -version "[16.0,18.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -sort -property installationPath > "%TEMP%\vspaths.txt" 2>nul
popd

for /f "usebackq delims=" %%i in ("%TEMP%\vspaths.txt") do (
    if not defined VS_PATH (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VS_PATH=%%i"
    )
)
del "%TEMP%\vspaths.txt" 2>nul

if not defined VS_PATH goto no_vs_path

REM Preserve user's VCPKG_ROOT across vcvars64.bat, which may set its own.
set "USER_VCPKG_ROOT=%VCPKG_ROOT%"
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 goto vcvars_failed
if defined USER_VCPKG_ROOT set "VCPKG_ROOT=%USER_VCPKG_ROOT%"
goto vs_ready

:no_vswhere
echo [X] vswhere.exe nao encontrado. Instale o Visual Studio 2019/2022.
exit /b 1

:no_vs_path
echo [X] Toolchain MSVC C++ nao encontrado. Instale o workload "Desktop development with C++".
exit /b 1

:vcvars_failed
echo [X] Falha ao carregar vcvars64.bat.
exit /b 1

:vs_ready

REM ==========================================
REM 2. Toolchain do vcpkg (se VCPKG_ROOT estiver definido)
REM ==========================================
set "TOOLCHAIN_ARG="
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo [*] Usando vcpkg toolchain: %VCPKG_ROOT%
        set TOOLCHAIN_ARG=-DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows
    ) else (
        echo [!] VCPKG_ROOT definido mas vcpkg.cmake nao encontrado em %VCPKG_ROOT%.
    )
) else (
    echo [!] VCPKG_ROOT nao definido - dependencias precisam estar no CMAKE_PREFIX_PATH.
)

REM ==========================================
REM 3. Configurar projeto com CMake + Ninja
REM ==========================================
if not exist "%BUILD_DIR%" (
    echo [*] Configurando projeto...
) else (
    echo [*] Verificando alteracoes de configuracao...
)

cmake -B %BUILD_DIR% -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% %TOOLCHAIN_ARG%
if errorlevel 1 (
    echo [X] Falha na configuracao do CMake.
    exit /b 1
)

REM ==========================================
REM 4. Compilar com Ninja
REM ==========================================
echo.
echo [*] Compilando com Ninja...
ninja -C %BUILD_DIR%
if errorlevel 1 (
    echo.
    echo [X] Falha na compilacao.
    exit /b 1
)

REM ==========================================
REM 5. Executar
REM ==========================================
echo.
echo [OK] Build Sucesso! Executando...
echo.
pushd %BUILD_DIR%
.\model_viewer.exe
popd

endlocal
