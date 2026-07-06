# Changelog

## v0.1.0-localdepsfix

- Fixed local CMake bootstrap so the official OBS plugin template includes `compilerconfig`, `defaults`, and `helpers` before `find_package(libobs REQUIRED)`.
- Local build can now trigger the buildspec-driven OBS dependency setup instead of failing with missing `libobsConfig.cmake`.

## v0.1.0

Initial MVP scaffold:

- Native OBS video filter source
- ArVisual one-pass GPU color enhancer shader
- Preset-based UX for non-colorists
- Windows local one-click build script
- Windows ZIP package script
- GitHub Actions Windows build/release workflow
- GitHub CLI public repo helper

## 0.1.0-installcheck

- Improved Windows package installer to request administrator permission.
- Added post-install verification for `arvisual.dll`, effect file, and locale file.
- Added `tools/diagnose-arvisual-obs.bat` to inspect OBS install path and latest OBS log for ArVisual load errors.
