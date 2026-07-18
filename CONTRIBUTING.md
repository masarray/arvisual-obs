# Contributing to ArVisual

Thank you for helping improve ArVisual. The project aims to provide a polished, creator-friendly OBS color enhancer without turning the filter into a dense professional grading panel.

## Product principles

Changes should preserve these priorities:

1. **Simple first-use experience** — presets and clearly named controls must remain useful to non-colorists.
2. **Neutral and skin safety** — visual impact must not come from blanket brightness or saturation.
3. **Predictable rendering** — no black output, flicker, stale shader mismatch, invalid gamut values or avoidable GPU stalls.
4. **Compact UI** — avoid bulky panels, oversized typography and always-visible advanced controls.
5. **Measured improvement** — visual tuning should be supported by representative scenes and regression checks.

## Before opening a pull request

- Search existing issues and pull requests.
- Keep each pull request focused on one problem or feature.
- Build the plugin in Release configuration.
- Run available tests and packaging validation.
- Test the installed package in OBS, not only the build output directory.
- Update documentation and `CHANGELOG.md` when behavior changes.

## Visual validation set

At minimum, test:

- webcam with a human face;
- product or toy scene with strong primary colors;
- white table, white wall or gray workbench;
- game capture with saturated UI elements;
- dark or low-key scene;
- low-light noisy source;
- transparent or letterboxed source;
- fast motion at the target frame rate.

Check for:

- neutral color cast;
- skin becoming orange, red or over-smoothed;
- clipped or flat highlights;
- crushed shadows;
- hue drift in vivid colors;
- clarity halos;
- black output or effect bypass;
- performance regression at 1080p60.

## Windows development

Requirements:

- Windows 10 or 11 x64;
- Visual Studio 2022 or newer with **Desktop development with C++**;
- CMake and Git;
- internet access for the first dependency bootstrap.

Run:

```bat
build-local-windows.bat
```

The generated manual package is placed in `release/`.

## Commit and pull request style

Use an imperative, specific commit subject, for example:

```text
Fix neutral highlight compression in clarity pass
```

A pull request should explain:

- the user-visible problem;
- the root cause;
- the implementation approach;
- validation performed;
- screenshots, logs or numerical results when relevant;
- known limitations.

## Installer changes

Installer changes must be tested for:

- standard system-wide OBS installation;
- per-user OBS installation when available;
- portable OBS selected manually;
- OBS not installed;
- invalid folder selection;
- OBS running during install;
- upgrade over an existing ArVisual version;
- uninstall behavior.

The OBS root is valid only when the expected executable and plugin data directories are present.

## License

By contributing, you agree that your contribution is distributed under the repository’s GPL-2.0-or-later license.
