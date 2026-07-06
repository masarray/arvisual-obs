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

## Downloads

Open the GitHub **Releases** page and choose the package for your OS.

### Windows

Recommended for normal users:

- `ArVisual-OBS-Setup-vX.X.X-windows-x64.exe`

Manual copy package:

- `arvisual-obs-vX.X.X-windows-x64.zip`

Manual install path:

```text
C:\Program Files\obs-studio\obs-plugins\64bit\arvisual.dll
C:\Program Files\obs-studio\data\obs-plugins\arvisual\effects\arvisual.effect
C:\Program Files\obs-studio\data\obs-plugins\arvisual\locale\en-US.ini
```

### macOS

Recommended for normal users:

- `ArVisual-OBS-Installer-vX.X.X-macos-universal.pkg`

Manual copy package:

- `arvisual-obs-vX.X.X-macos-universal-manual.zip`

Manual install path:

```text
~/Library/Application Support/obs-studio/plugins/arvisual.plugin
```

### Linux

Recommended for Debian/Ubuntu-style users:

- `arvisual-obs-vX.X.X-linux-x86_64.deb`

Manual package:

- `arvisual-obs-vX.X.X-linux-x86_64-manual.tar.gz`

Typical install paths:

```text
/usr/lib/x86_64-linux-gnu/obs-plugins/
/usr/share/obs/obs-plugins/arvisual/
```

## Add the filter in OBS

1. Restart OBS Studio after installing.
2. Right-click a video source.
3. Open **Filters**.
4. Click **+**.
5. Choose **ArVisual - Smart Color Enhancer**.
6. Start with **Kids Toy Pop** or **MasAri Dopamine**.

## Local one-click Windows build

Requirements:

- Windows 10/11 x64
- Visual Studio 2026 or Visual Studio 2022 with **Desktop development with C++**
- CMake
- Git
- Internet connection for the first build

Then double-click:

```bat
build-local-windows.bat
```

The script clones the official OBS plugin template into `.build/`, overlays the ArVisual source, builds the plugin, then creates a user-ready ZIP in `release/`.

To force Visual Studio 2026:

```bat
build-local-windows-vs2026.bat
```

## Automated release workflow

This repo has a cross-platform GitHub Actions release workflow:

```text
.github/workflows/release.yml
```

To create a public release page with all packages attached:

```powershell
git tag v0.1.0
git push origin v0.1.0
```

The workflow builds:

| OS | User installer | Manual package |
|---|---|---|
| Windows x64 | `.exe` via Inno Setup | `.zip` |
| macOS universal | `.pkg` | `.zip` |
| Linux x86_64 | `.deb` | `.tar.gz` |

There is also a legacy/manual Windows CI workflow:

```text
.github/workflows/build-windows.yml
```

It is workflow-dispatch only, so tag releases are handled by `release.yml`.

## Current state

This is an MVP-native OBS filter. It is designed to build from source and give a real-time one-pass shader look. The first public validation should focus on:

- OBS loads the plugin successfully
- filter appears under source filters
- presets apply values correctly
- no rendering flicker
- no harsh skin-tone orange shift
- 1080p60 performance stays comfortable

## Troubleshooting

### Filter does not appear

Check that the plugin files exist in the active OBS installation folder. Then open:

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

### CMake cannot find Visual Studio

CMake is only the build generator. You also need a C++ compiler/toolchain. Install Visual Studio 2026/2022 with **Desktop development with C++**, then run the build again.

### CMake cannot find libobsConfig.cmake

First local build downloads OBS dependency archives and builds the OBS development libraries through the official OBS plugin template buildspec flow. If this fails, delete `.build/` and run `build-local-windows.bat` again.

## License

GPL-2.0-or-later.
