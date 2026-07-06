# ArVisual Smart Color Enhancer for OBS

ArVisual is a native OBS video filter that makes a camera, product shot, game source, or video source look cleaner, brighter, more colorful, and more pleasing with one click.

It is designed for non-colorists. Instead of complex lift/gamma/gain controls, ArVisual gives simple visual controls such as **Color Pop**, **Clean White**, **Visual Clarity**, **Skin Protect**, and ready-to-use presets.

## Target visual style

ArVisual targets a **high-key dopamine color** look:

- cleaner whites and brighter midtones
- stronger toy/product colors
- vivid red, yellow, blue, green, cyan, and magenta zones
- protected skin tones
- crisp luma-only clarity, not harsh RGB sharpening
- glossy highlights without obvious clipping

## Presets

| Preset | Best for |
|---|---|
| MasAri Dopamine | Default all-round pop color |
| Kids Toy Pop | Children/toy/product scenes, clean white, glossy color |
| Cars Candy Color | Strong animation-like candy colors |
| Product Clean Pop | Product review, unboxing, audio gear, gadgets |
| Beauty Bright | Webcam and talking-head sources |
| Gaming Vivid | Colorful games and screen capture |
| Natural Safe | Meeting, class, conservative enhancement |
| Cinematic Candy | Pop color with a softer filmic feel |

## Install prebuilt ZIP

1. Download `arvisual-obs-vX.X.X-windows-x64.zip` from GitHub Releases.
2. Close OBS Studio.
3. Extract the ZIP.
4. Copy the extracted `obs-plugins` and `data` folders into your OBS install folder.
   - Default: `C:\Program Files\obs-studio`
5. Open OBS.
6. Right-click a source → **Filters** → **+** → **ArVisual - Smart Color Enhancer**.
7. Start with **Kids Toy Pop** or **MasAri Dopamine**.

The ZIP also includes `install-to-obs-windows.bat` for a simple default-path install.

## Local one-click Windows build

Requirements:

- Windows 10/11 x64
- Visual Studio 2022 with **Desktop development with C++**
- CMake
- Git
- Internet connection for the first build

Then double-click:

```bat
build-local-windows.bat
```

The script clones the official OBS plugin template into `.build/`, overlays the ArVisual source, builds the plugin, then creates a user-ready ZIP in `release/`.

## Create public GitHub repo

This repository includes a helper script that uses GitHub CLI:

```bat
create-public-repo-github.bat
```

Before running it:

```powershell
gh auth login
```

By default it creates/pushes to:

```text
masarray/arvisual-obs
```

You can edit `scripts/create-public-repo-gh.ps1` to use another owner or repo name.

## Release workflow

The GitHub Actions workflow builds Windows x64 and uploads a ZIP artifact on every push. To create a public release:

```powershell
git tag v0.1.0
git push origin v0.1.0
```

A GitHub Release will be created with the Windows ZIP attached.

## Current state

This is an MVP-native OBS filter. It is designed to build from source and give a real-time one-pass shader look. The first public validation should focus on:

- OBS loads the plugin successfully
- filter appears under source filters
- presets apply values correctly
- no rendering flicker
- no harsh skin-tone orange shift
- 1080p60 performance stays comfortable

## License

GPL-2.0-or-later.


## Windows build requirement note

`build-local-windows.bat` uses the CMake preset `windows-x64`, which uses the CMake generator `Visual Studio 17 2022`. CMake from cmake.org is fine, but it is only the project generator. You still need Visual Studio 2022 Community/Professional/Enterprise or Visual Studio Build Tools 2022 with the **Desktop development with C++** workload.

If you see this error:

```text
Generator Visual Studio 17 2022 could not find any instance of Visual Studio.
```

Install the C++ build tools, then run the build again:

```text
Right-click install-vs2022-buildtools.bat -> Run as administrator
```

Manual path:

```text
Visual Studio Installer -> Modify -> Desktop development with C++ -> Install
```


## Visual Studio 2026 / 2022 local build

The local Windows build script auto-detects Visual Studio 2026 first, then falls back to Visual Studio 2022. Use:

```bat
build-local-windows.bat
```

To force Visual Studio 2026:

```bat
build-local-windows-vs2026.bat
```

Important: CMake must list the matching generator. Check with:

```bat
cmake --help | findstr /C:"Visual Studio"
```

For VS 2026, CMake must show `Visual Studio 18 2026`. For VS 2022, it must show `Visual Studio 17 2022`.


## Local build note

This package includes a vswhere.exe path fix for Windows PowerShell. If the build previously printed `vswhere found: C`, replace the repository with this package or copy `scripts/check-windows-build-env.ps1` from this version.


## Local build note

First local build downloads OBS dependency archives and builds the OBS development libraries through the official OBS plugin template buildspec flow. This is expected. Later builds reuse the local `.build/obs-plugintemplate/.deps` cache.

If you see `Could not find libobsConfig.cmake`, use the latest ArVisual ZIP where `CMakeLists.txt` includes `compilerconfig`, `defaults`, and `helpers` before `find_package(libobs REQUIRED)`.
