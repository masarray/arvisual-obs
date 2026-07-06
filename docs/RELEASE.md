# Release Process

## Manual release

```powershell
git add .
git commit -m "Release v0.1.0"
git tag v0.1.0
git push origin main --tags
```

The workflow `.github/workflows/build-windows.yml` builds the Windows x64 ZIP and attaches it to the GitHub Release.

## Local package

```bat
build-local-windows.bat
```

Output:

```text
release/arvisual-obs-v0.1.0-windows-x64.zip
```
