# ArVisual Engine Notes

## Design target

ArVisual is not a manual color grading panel. It is a perceptual visual enhancer for OBS users who want fast, good-looking results.

## Pipeline

```text
Input OBS texture
  -> high-key tone curve
  -> smart vibrance
  -> skin-tone protection
  -> toy/object hue-zone pop
  -> clean-white cast reduction
  -> luma-only clarity
  -> glossy highlight guard
  -> gamut safety
  -> OBS output texture
```

## Why one-pass shader first

The MVP uses one GPU shader pass to stay light for livestreaming. CPU involvement is limited to setting UI parameters and source dimensions.

## Controls

- Smart Enhance: overall high-key visual enhancement.
- Color Pop: perceptual vibrance plus object hue-zone pop.
- Clean White: bright neutral cast cleanup.
- Visual Clarity: luma-only edge/detail boost.
- Skin Protect: limiter for orange/red-yellow skin zones.
- Toy Gloss: small highlight lift for plastic/product shine.
- Highlight Guard: smooth rolloff to avoid clipped neon results.
- Performance: changes internal clarity intensity.

## Known limitations

- This MVP does not do AI face detection.
- Skin protection is hue/luma/saturation based, not semantic segmentation.
- HDR/Rec.2100 tone mapping is not implemented yet.
- Very bad lighting, clipped highlights, and heavy camera noise cannot be fully recovered.
