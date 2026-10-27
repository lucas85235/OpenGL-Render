@echo off
setlocal EnableDelayedExpansion

REM Half Loop runner for Claude Code (Windows cmd).
REM A skill /half le CLAUDE.md + progress.txt e executa 1 task por iteracao.
REM
REM Uso:
REM   scripts\half\h.bat                # usa "claude" (default)
REM   scripts\half\h.bat --tool codex   # usa outro CLI compativel
REM   set MAX_ITERS=100 ^& scripts\half\h.bat

set "TOOL=claude"
if not defined MAX_ITERS set "MAX_ITERS=50"

:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--tool" ( set "TOOL=%~2" & shift & shift & goto parse_args )
if "%~1"=="--max"  ( set "MAX_ITERS=%~2" & shift & shift & goto parse_args )
echo arg desconhecido: %~1
exit /b 2
:args_done

set "SCRIPT_DIR=%~dp0"
pushd "%SCRIPT_DIR%\..\.."
set "ROOT_DIR=%cd%"

if not exist CLAUDE.md (
    echo CLAUDE.md nao encontrado em %ROOT_DIR%
    popd
    exit /b 1
)
if not exist progress.txt (
    echo No tasks completed yet. Start from Task 1.1.> progress.txt
)

echo === Half Loop start (tool=%TOOL% max=%MAX_ITERS% root=%ROOT_DIR%) ===

set /a i=0
:loop
set /a i+=1
if %i% GTR %MAX_ITERS% goto max_hit

findstr /C:"ALL TASKS COMPLETE" progress.txt >nul 2>&1
if not errorlevel 1 (
    echo [half] ALL TASKS COMPLETE detected. Stopping at iter %i%.
    popd
    exit /b 0
)

echo.
echo === Iter %i% / %MAX_ITERS% ===

"%TOOL%" --dangerously-skip-permissions -p "/half"
set "RC=%errorlevel%"

if not "%RC%"=="0" (
    echo [half] %TOOL% exit code %RC%. Abortando.
    popd
    exit /b %RC%
)

goto loop

:max_hit
echo [half] MAX_ITERS (%MAX_ITERS%) atingido sem ALL TASKS COMPLETE.
popd
exit /b 1
