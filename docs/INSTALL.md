# Installation Guide

## Normal OBS Studio install

1. Close OBS Studio.
2. Extract `arvisual-obs-vX.X.X-windows-x64.zip`.
3. Copy these two folders into the OBS install root:
   - `obs-plugins`
   - `data`
4. Default OBS root:

```text
C:\Program Files\obs-studio
```

5. Open OBS Studio.
6. Right-click a source → Filters → + → ArVisual - Smart Color Enhancer.

## Portable OBS

Copy the same `obs-plugins` and `data` folders into the root folder of your portable OBS.

## Uninstall

Delete:

```text
C:\Program Files\obs-studio\obs-plugins\64bit\arvisual.dll
C:\Program Files\obs-studio\data\obs-plugins\arvisual
```


## Fix: CMake cannot find Visual Studio 2022

This error means CMake is installed, but the Visual Studio 2022 C++ compiler/build tools are not installed or not discoverable:

```text
Generator Visual Studio 17 2022 could not find any instance of Visual Studio.
```

Fix:

1. Run `install-vs2022-buildtools.bat` as Administrator, or install Visual Studio 2022 manually.
2. In Visual Studio Installer, select **Desktop development with C++**.
3. Restart Command Prompt/PowerShell.
4. Run `build-local-windows.bat` again.

The local build script now checks for `git`, `cmake`, `vswhere`, Visual Studio 2022/Build Tools, MSBuild, MSVC, and Windows SDK before running the OBS plugin template build.


## Visual Studio 2026 support

ArVisual supports local build with Visual Studio Community 2026 when CMake supports the `Visual Studio 18 2026` generator. The one-click `build-local-windows.bat` auto-selects VS 2026 first.

Use this command to verify generator support:

```bat
cmake --help | findstr /C:"Visual Studio"
```

If `Visual Studio 18 2026` is missing, update CMake or use the CMake bundled with Visual Studio 2026 Developer PowerShell.

Manual force-build for VS 2026:

```bat
build-local-windows-vs2026.bat
```


## Missing libobs / libobsConfig.cmake

If CMake reports that it cannot find `Findlibobs.cmake`, `libobsConfig.cmake`, or `libobs-config.cmake`, make sure you are using the patched ArVisual repo that includes the OBS plugin template bootstrap in `CMakeLists.txt`:

```cmake
include(compilerconfig)
include(defaults)
include(helpers)
```

Those template modules are required for local one-click builds. They download/extract the OBS dependencies from `buildspec.json`, build/install the libobs Development component into `.build/obs-plugintemplate/.deps`, and populate `CMAKE_PREFIX_PATH` before ArVisual calls `find_package(libobs REQUIRED)`.

First configure can take longer because it downloads OBS source/dependency archives and builds the OBS development libraries. Later builds reuse the `.deps` cache.

## Filter does not appear after install

Check these files in your real OBS installation folder:

```text
obs-plugins\64bit\arvisual.dll
data\obs-plugins\arvisual\effects\arvisual.effect
data\obs-plugins\arvisual\locale\en-US.ini
```

If any file is missing, run `install-to-obs-windows.bat` from the packaged release as administrator.

If all files exist but the filter is missing, restart OBS and open:

```text
OBS -> Help -> Log Files -> View Current Log
```

Search for:

```text
arvisual
arvisual.dll
Failed to load
```

Expected success line:

```text
[ArVisual] Smart Color Enhancer loaded
```

No `arvisual` log line usually means the files were copied to the wrong OBS folder, such as Program Files while you are actually running a portable or Steam OBS.
