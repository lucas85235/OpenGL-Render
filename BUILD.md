# Build Requirements

This project builds on both Linux and Windows using CMake + Ninja.
The executable produced is `model_viewer` (`model_viewer.exe` on Windows).

## Common requirements (all platforms)

| Tool              | Purpose                              |
| ----------------- | ------------------------------------ |
| CMake >= 3.12     | Build configuration                  |
| Ninja             | Build generator                      |
| C++17 compiler    | Language features / `std::filesystem`|
| Python 3          | Unified shader preprocessor (`tools/compile_shaders.py`) |
| Vulkan SDK        | Provides `glslc` (SPIR-V shader compilation) |
| Git               | Source + fetching `stb_image.h`      |

## Project libraries

The following libraries are linked by `CMakeLists.txt` via `find_package`:

- **OpenGL** (system)
- **Vulkan** (from Vulkan SDK)
- **GLEW**
- **GLFW3** (>= 3.3)
- **GLM**
- **Assimp**
- **Threads** (pthreads / Win32 threads)
- **SPIRV-Cross** *(optional, enable with `-DUSE_SPIRV_CROSS=ON`)*
- **Dear ImGui** (docking branch) — vendored at `src/vendor/imgui`
- **stb_image** — downloaded automatically by CMake on first configure

---

## Linux

Tested on Ubuntu/Debian. Package names may differ on other distros.

### Install toolchain

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build python3 git
```

### Install libraries

```bash
sudo apt install -y \
    libgl1-mesa-dev \
    libglew-dev \
    libglfw3-dev \
    libglm-dev \
    libassimp-dev \
    glslang-tools
```

### Install Vulkan SDK

Follow the instructions at [vulkan.lunarg.com](https://vulkan.lunarg.com/sdk/home#linux)
or install the distro package:

```bash
sudo apt install -y libvulkan-dev vulkan-tools
```

### Optional: SPIRV-Cross

```bash
sudo apt install -y libspirv-cross-c-shared-dev
# Then build with: cmake -DUSE_SPIRV_CROSS=ON ...
```

### Build

```bash
./build.sh
```

The script configures with CMake/Ninja in `build/`, compiles, and runs
`./build/model_viewer`.

---

## Windows

Tested on Windows 10/11 with Visual Studio 2022 Community.

### 1. Install Visual Studio 2022 (or 2019) Build Tools

Download from https://visualstudio.microsoft.com/downloads/ and install the
**"Desktop development with C++"** workload. This provides:

- MSVC compiler (`cl.exe`)
- Windows SDK
- CMake (bundled)
- Ninja (bundled)

The `build.bat` script auto-detects the install via `vswhere.exe` and runs
`vcvars64.bat` automatically — you do **not** need to open a Developer Prompt.

> If you prefer system-wide tools, install CMake and Ninja separately:
> ```powershell
> winget install Kitware.CMake
> winget install Ninja-build.Ninja
> ```

### 2. Install Python 3

```powershell
winget install Python.Python.3.12
```

Needed by `tools/compile_shaders.py`.

### 3. Install the Vulkan SDK

Download from https://vulkan.lunarg.com/sdk/home#windows and run the installer.
It sets the `VULKAN_SDK` environment variable, which CMake uses to locate
`glslc.exe`.

### 4. Install libraries via vcpkg

OpenGL comes with Windows and Vulkan comes with the SDK, but GLEW, GLFW3,
GLM, and Assimp must be installed. The easiest path is
[vcpkg](https://github.com/microsoft/vcpkg):

```powershell
# 1. Clone and bootstrap vcpkg (anywhere you like — C:\vcpkg is conventional)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat -disableMetrics

# 2. Install the libraries for the x64-windows triplet
C:\vcpkg\vcpkg.exe install glew glfw3 glm assimp --triplet x64-windows

# 3. (Optional) Install SPIRV-Cross if you want -DUSE_SPIRV_CROSS=ON
C:\vcpkg\vcpkg.exe install spirv-cross --triplet x64-windows
```

### 5. Point the build script at vcpkg

Set `VCPKG_ROOT` so `build.bat` picks up the toolchain automatically:

```powershell
# Persistently (recommended):
setx VCPKG_ROOT "C:\vcpkg"

# Or for one shell only:
$env:VCPKG_ROOT = "C:\vcpkg"
```

Open a new terminal after `setx` so the variable is visible.

### 6. Build

From the project root:

```cmd
build.bat
```

The script:

1. Loads the MSVC environment (via `vswhere` + `vcvars64.bat`)
2. Passes the vcpkg CMake toolchain if `VCPKG_ROOT` is set
3. Configures the project with Ninja into `build\`
4. Compiles
5. Runs `build\model_viewer.exe`

---

## Troubleshooting

### CMake cannot find GLEW/GLFW/Assimp on Windows

- Confirm `VCPKG_ROOT` is set in **the same shell** you run `build.bat` from.
- Confirm the triplet matches: the script uses `x64-windows`. Install with
  `--triplet x64-windows`.
- Re-run after `setx` only in a **new** terminal — `setx` doesn't affect the
  current shell.

### `glslc` / `glslangValidator` not found

Install the Vulkan SDK and make sure `VULKAN_SDK` is set (Windows) or
`glslang-tools` is installed (Linux).

### `polyclipping` download fails when installing assimp via vcpkg

vcpkg pulls `polyclipping` (an assimp dep) from SourceForge, which occasionally
returns SSL errors. Retry the install — it usually succeeds on the second
attempt:

```powershell
C:\vcpkg\vcpkg.exe install assimp --triplet x64-windows
```

### `build.bat` picks the wrong Visual Studio install

If you have multiple Visual Studio / Build Tools side-by-side (e.g. VS 2022
Community *and* a VS Build Tools preview), `build.bat` filters to the
`[16.0, 18.0)` version range (VS 2019–VS 2022). Previews above VS 2022
(version `17`) are skipped on purpose — their bundled `vcvars64.bat` may call
`vswhere.exe` from unexpected locations and fail. Keep using VS 2019/2022 for
reliable builds.

### `'vswhere.exe' is not recognized as an internal or external command`

This message leaks from `vcvars64.bat` internally when the Visual Studio
Installer directory is not on `PATH`. The build still works because the
script resolves the install path itself — the message is cosmetic and can be
ignored.

### `VCPKG_ROOT` seems to change after build starts

`vcvars64.bat` sets its own `VCPKG_ROOT` pointing at the VS-bundled vcpkg.
`build.bat` saves your `VCPKG_ROOT` before calling `vcvars64.bat` and restores
it afterward, so as long as you ran `build.bat` with your `VCPKG_ROOT`
already in the environment, it will be respected.

### `M_PI` undeclared on MSVC

MSVC does not define `M_PI` in `<cmath>` by default. The `CMakeLists.txt`
defines `_USE_MATH_DEFINES` globally for MSVC builds to fix this.

### Missing shaders at runtime

CMake copies `src/shaders` and `models/` into `build/`. If you move the
executable elsewhere, copy those folders alongside it, or run from `build/`.

### Runtime `DLL not found` errors on Windows

vcpkg's CMake integration runs `applocal.ps1` after linking to copy required
runtime DLLs (GLEW, GLFW, etc.) next to `model_viewer.exe`. It scans the
binary's imports — if you later add code that dynamically loads a library
(`LoadLibrary`), you may need to copy its DLL manually from
`%VCPKG_ROOT%\installed\x64-windows\bin\`.
