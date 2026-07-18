## Problem

Describe the user-visible problem or maintenance risk.

## Solution

Explain the smallest focused change and why it is appropriate for ArVisual.

## Validation

- [ ] `python scripts/validate-repository.py`
- [ ] `python scripts/validate-color-safety.py`
- [ ] Windows installer script compiles when installer files changed
- [ ] Tested in OBS Studio when rendering, presets, packaging, or installation changed
- [ ] Documentation and changelog updated when behavior changed

## Visual and performance checks

- [ ] Whites and grays remain neutral
- [ ] Skin remains believable
- [ ] Highlights retain texture
- [ ] No black output, flicker, or obvious clarity halo
- [ ] 1080p60 performance checked when the render path changed

## Release impact

State whether this requires a version bump, new package, migration note, or no release action.
