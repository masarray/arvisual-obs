# ArVisual Quick Start

ArVisual is a native OBS Studio source filter. Install it into the active OBS installation, restart OBS, then add it to a video source.

## Windows x64 installer

1. Open the [latest release](https://github.com/masarray/arvisual-obs/releases/latest).
2. Download `ArVisual-OBS-Setup-vX.X.X-windows-x64.exe`.
3. Close OBS Studio completely.
4. Run the installer as prompted.
5. Confirm the detected OBS Studio root folder.
6. Finish installation and reopen OBS Studio.

The installer searches registered and common OBS locations. An existing OBS directory is expected and does not trigger a generic folder warning. When OBS cannot be detected, the installer asks for the root folder and validates that it contains:

```text
bin\64bit\obs64.exe
obs-plugins\64bit\
data\obs-plugins\
```

## Windows manual install

Download `arvisual-obs-vX.X.X-windows-x64.zip` and extract its contents into the OBS Studio root.

Expected files:

```text
C:\Program Files\obs-studio\obs-plugins\64bit\arvisual.dll
C:\Program Files\obs-studio\data\obs-plugins\arvisual\effects\arvisual.effect
C:\Program Files\obs-studio\data\obs-plugins\arvisual\locale\en-US.ini
```

## macOS

Use the release package named:

```text
ArVisual-OBS-Installer-vX.X.X-macos-universal.pkg
```

The manual package contains `arvisual.plugin`. Copy it to:

```text
~/Library/Application Support/obs-studio/plugins/
```

## Linux

For Debian or Ubuntu-style systems, use:

```text
arvisual-obs-vX.X.X-linux-x86_64.deb
```

The manual archive can be copied into the matching plugin and data paths used by the installed OBS package. Common locations include:

```text
/usr/lib/x86_64-linux-gnu/obs-plugins/
/usr/share/obs/obs-plugins/arvisual/
```

## Add ArVisual to a source

1. Open OBS Studio.
2. Right-click a camera, game capture, display capture, image or media source.
3. Select **Filters**.
4. Under **Effect Filters**, click **+**.
5. Select **ArVisual - Smart Color Enhancer**.
6. Choose a preset.
7. Adjust **Instant Amount** first.

## Recommended starting points

| Source | Preset |
|---|---|
| Product review, unboxing, audio gear | Product Clean Pop |
| General colorful content | MasAri Dopamine |
| Webcam or presenter | Beauty Bright |
| Game capture | Gaming Vivid |
| Classroom, meeting or conservative scene | Natural Safe |
| Toys and colorful learning content | Kids Toy Pop |

## First checks

After adding the filter, verify:

- the source remains visible with no black output;
- whites and gray surfaces remain neutral;
- skin does not become strongly orange or red;
- highlights retain texture;
- motion and 1080p60 rendering remain comfortable on the target system.

See [Troubleshooting](TROUBLESHOOTING.md) when the filter does not appear or OBS reports a loading error.
