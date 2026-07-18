# Troubleshooting ArVisual for OBS

## Filter does not appear

Confirm that the plugin files are installed in the same OBS Studio installation that you are running.

Windows x64 expected paths:

```text
<OBS root>\obs-plugins\64bit\arvisual.dll
<OBS root>\data\obs-plugins\arvisual\effects\arvisual.effect
<OBS root>\data\obs-plugins\arvisual\locale\en-US.ini
```

Then restart OBS Studio and open:

```text
Help → Log Files → View Current Log
```

Search for:

```text
arvisual
arvisual.dll
Failed to load
```

Expected success message:

```text
[ArVisual] Smart Color Enhancer loaded
```

## Installer says OBS was not detected

The installer could not confirm a registered or common OBS installation path.

Select the OBS Studio root folder—not `bin`, `obs-plugins`, or the executable folder. A valid Windows x64 root contains:

```text
bin\64bit\obs64.exe
obs-plugins\64bit\
data\obs-plugins\
```

Portable OBS installations are supported when their root folder is selected manually.

## Installer refuses the selected directory

The selected folder failed validation. Common mistakes:

- selecting `C:\Program Files` instead of `C:\Program Files\obs-studio`;
- selecting `bin\64bit` instead of the OBS root;
- selecting a folder from an old or incomplete OBS copy;
- selecting a 32-bit OBS layout while using the Windows x64 ArVisual package.

## OBS is running during installation

Close OBS Studio before installing or upgrading. The native DLL can remain locked while OBS is open, which may leave an older plugin version in place.

Use Task Manager to confirm that `obs64.exe` is no longer running, then start the installer again.

## Source turns black or the filter bypasses itself

ArVisual validates the native plugin and effect ABI. A mismatched DLL and effect can be bypassed to avoid an invalid render path.

Remove the old ArVisual files, reinstall the complete package from one release, and restart OBS. Do not combine a DLL from one version with an effect file from another version.

## Colors are too strong

1. Reduce **Instant Amount**.
2. Try **Natural Safe** or **Product Clean Pop**.
3. Keep **Smart Auto** enabled for scene-dependent restraint.
4. Check whether another OBS color filter is applied before or after ArVisual.

## Skin looks too warm

Reduce **Healthy Tone** or **Skin Beauty**, then compare against **Natural Safe**. Mixed lighting or an incorrect camera white balance should be corrected at the camera or source level first.

## Performance is lower than expected

- Select a lighter ArVisual performance tier.
- Test at the actual stream or recording resolution and frame rate.
- Disable duplicate enhancement filters on the same source.
- Update the GPU driver.
- Compare with ArVisual bypassed before reporting a regression.

## Build cannot find Visual Studio

CMake is the build generator; it still requires a C++ compiler toolchain.

Install Visual Studio with:

```text
Desktop development with C++
```

Then rerun `build-local-windows.bat`.

## Build cannot find libobsConfig.cmake

Delete the local `.build/` directory and rerun the one-click build so the pinned OBS dependencies and plugin template are bootstrapped again.

## Reporting a useful bug

Include:

- ArVisual version;
- OBS Studio version;
- operating system and architecture;
- GPU model;
- exact installation method;
- relevant OBS log lines;
- reproduction steps;
- a screenshot or short sample when the issue is visual.

Use the repository [bug report form](https://github.com/masarray/arvisual-obs/issues/new/choose).
