# Changelog

## Unreleased

- Reworked the Windows installer as an OBS-aware plugin installer: existing OBS folders no longer trigger Inno Setup's generic directory warning.
- Added automatic OBS discovery through registered application paths, uninstall metadata and common installation locations.
- Added strict OBS root validation for standard and portable installations; the installer now warns only when OBS cannot be detected or the selected folder is invalid.
- Added a professional README, responsive GitHub Pages landing page, SEO metadata, quick-start and troubleshooting documentation, contribution guidance and security reporting policy.

## v0.5.7

- Fixed the real diffuse-white dimming root cause: the clarity pass no longer applies a `0.84` shoulder to unchanged source luminance. Flat white now bypasses clarity tone compression while the final scene-aware shoulder still protects highlights.
- Excluded neutral pixels from the warm-light depth split, keeping whites and grays achromatic instead of introducing a subtle red bias.
- Rebalanced the neutral tone budget toward a stronger always-on restraint with a smaller Smart Clean supplement, reducing excessive mid-gray lift in colorful scenes without touching skin.
- Added scene-risk-aware saturation caps for green, teal, blue-violet, and lavender families. Warm hero colors retain their separate chroma-preservation path, so dopamine emphasis remains selective rather than blanket saturation.
- Tightened regression limits for diffuse white, neutral midtones, and six audited secondary ColorChecker anchors while retaining gamut, clipping, hue, skin, and product-scene invariants.

## v0.5.6

- Added scene-aware neutral-luma restraint so Smart Clean removes a cast without making large workbench, wall, or paper areas look clinically overexposed.
- Preserved diffuse white sparkle with a dedicated low-saturation highlight guard and a gentler neutral shoulder, while retaining the hard gamut and clipping invariants.
- Gated the cool shadow split by source chroma confidence, preventing genuinely neutral blacks and grays from acquiring a blue cast.
- Added warm-hero chroma retention after luminance separation so orange/red products gain presence without losing relative color purity; skin remains excluded from this path.
- Reduced blue saturation dose with hue-aware scene scaling, keeping cables and display accents vivid but below the audited oversaturation threshold.
- Tightened the numerical regression around the aligned real-video anchors for neutral lift, diffuse white, orange chroma, blue saturation, dark neutrality, skin stability, hue drift, gamut, and clipping.

## v0.5.5

- Added scene-aware product/tutorial classification using neutral-field and colored-subject fractions. Dominant neutral workspaces with sparse colored objects now receive stronger cleanup and object separation without blanket saturation.
- Added `smart_clean` and `smart_separation` adaptive outputs. Smart Clean removes residual green/cyan cast from neutral surfaces; Smart Separation allocates luminance contrast to warm hero objects, fresh cyan/green accents, and dense blues.
- Rebalanced real-video priorities from the audited Fluke frame: orange/red products receive roughly three times the previous hero-separation contribution, blue wiring gains density, while skin is excluded more strongly from vibrance and object-pop paths.
- Reduced positive skin tone lift and capped Healthy Tone saturation more tightly. Product-video regression holds the measured skin anchor near source (`dY +0.003`, small saturation movement) instead of lifting skin more strongly than the product.
- Bumped the DLL/effect contract to ABI v3 for the two new scene-adaptive uniforms; stale v0.5.4 shader/DLL combinations safely bypass.
- Extended the dependency-free regression with measured workbench, white, Fluke orange, red, blue wire, skin, and product-black anchors plus product-scene classification assertions.

## v0.5.4

- Professional dopamine finishing: vividness now comes from bounded warm/cool hero-luma separation plus controlled chroma, improving visual pleasure without turning every patch neon.
- Smart Auto now tracks P90 saturation, so a small vivid subject still triggers dose protection even when most of the frame is dark or neutral.
- Exposure now follows the P90/P98 upper key instead of the median-heavy scene key, preserving intentionally low-key photography and preventing large dark backgrounds from forcing maximum lift.
- Added toe- and shoulder-protected contrast. Deep shadow detail no longer collapses under the midtone contrast pivot, while diffuse neutral whites retain clean sparkle.
- Positive tone, skin, clarity, punch, and gloss lifts now consume channel headroom; warm skin highlights cannot be manufactured into a digital red-channel plateau.
- Gamut mapping now uses an inner display-safe boundary for generated colors while preserving genuine source black and white extremes.
- Saturation gain is capped per pixel and skin saturation has its own tighter cap. The measured ColorChecker model now averages `+0.039` saturation with maximum luma movement `0.029`.
- Expanded the regression suite with white-point retention, toe-detail preservation, hue stability, inner-gamut, saturation-gain, and low-key/P90-tail Smart Auto checks.

## v0.5.3

- Replaced unbounded additive HSV saturation with headroom-based chroma enhancement. Existing vivid colors are preserved; new color boost can never request saturation outside the adaptive ceiling.
- Added luminance-preserving gamut mapping after every tone, color, skin, clarity, and gloss stage. Invalid negative/over-range RGB is prevented before the final output instead of being hard-clipped afterward.
- Smart Auto now analyzes active-picture P10/P50/P90/P98 luminance plus saturation, vivid-area, hot-vivid, shadow, and near-clip fractions. Transparent pixels and near-black letterbox/background regions no longer create false exposure or color-pop decisions.
- Added adaptive creative-strength and chroma-ceiling controls so risky/highly saturated scenes automatically receive a lower dose, while muted scenes receive at most a very small lift.
- Reduced midtone, skin, clarity, depth, and gloss doses and routed their brightness changes through a soft highlight shoulder.
- Bumped the DLL/effect interface to ABI v2 so any stale v0.5.2 shader is safely bypassed instead of partially executing the new engine.
- Added a numerical ColorChecker safety regression covering gamut bounds, luma collapse, highlight creation, neutral preservation, and saturation monotonicity.

## v0.5.2

- Fixed blank/black output caused by mixed DLL and effect versions: Windows packages now fail unless the DLL, effect, and both locale files are present in the package and ZIP.
- The Windows installer now validates every source payload file and verifies the installed SHA-256 hashes, preventing stale effect/locale files from being reported as a successful install.
- Smart Auto capture now uses `OBS_NO_DIRECT_RENDERING`, guaranteeing a texture-backed input for the analysis and final passes.
- Added a shader ABI sentinel and required-parameter validation. An incompatible effect is logged and safely bypassed instead of reaching D3D11 with unset uniforms.

## v0.5.1 (Enhance+)

- **Deterministic color space**: the render path now pins non-linear (gamma / sRGB-encoded) sampling and writes via `gs_set_linear_srgb(false)` and `gs_enable_framebuffer_srgb(false)` around all draw paths. Previously the engine's input space depended on OBS global state, silently changing grade strength and character between OBS versions/setups. The engine is designed and tuned in gamma space; it now always runs there.
- **Vivid-bright protection**: pixels that are already bright AND saturated no longer receive midtone/upper-mid luma lift (and get a softened s-curve). Pushing them into the gamut ceiling is what destroys chroma on vivid yellow/orange/amber; they are bright enough - their chroma is now preserved.
- **Dark-vivid glow**: saturated colors in low-mid luma get a small dedicated lift, so strong colors pop out of dark scenes instead of sitting muted.
- **Enhance+ tuning** (all through the unchanged safety chain - shoulder, adaptive rolloff, chroma compression, soft-limited clarity): vibrance 0.42 -> 0.50, object-pop 0.155/0.060 -> 0.175/0.070, pop luma gate opened to darker colors (0.09..0.26 -> 0.055..0.20), saturation gate widened (0.82 -> 0.86), upper-mid depth lift 0.040 -> 0.048, shadow anchor 0.018 -> 0.022, warm/cool split 0.018/0.014 -> 0.022/0.017, clarity presence 0.155 -> 0.165, perceptual punch 0.035 -> 0.045.
- Numerical validation via a NumPy port of the pixel pipeline on 24 measured chart colors: colored-patch mean saturation gain +0.126 -> +0.150, neutral patches stay at ~+0.004, vivid yellow keeps chroma (+0.044 instead of being pushed into the ceiling), max output channel 0.985 (no clipping).

## v0.5.0

True Smart Engine release.

- **Smart Auto (Scene Adaptive)**: the filter now captures its input into a texrender, downsamples to 64x36, and reads it back on the CPU (ping-pong stage surfaces, 1-frame latency, no GPU stall). Mean luma, mean chroma, highlight fraction and shadow fraction are EMA-smoothed (tau 0.7 s) and drive four bounded adaptive corrections: gentle exposure toward a healthy midtone key, automatic Color Pop trim on already-saturated scenes (and lift on muted scenes), extra highlight shoulder pressure on hot scenes, and shadow-anchor easing on crushed scenes. Toggleable; off = fully neutral.
- **Anti-halo clarity**: detail is soft-limited with a rational curve, strong edges get an explicit halo-guard rolloff, and positive (brightening) detail is damped near highlights so clarity never draws bright rims.
- **Luma pre-shoulder**: tone-stage pushes above 0.86 land on a rational shoulder instead of the hard clip; the shoulder tightens under smart highlight pressure. Highlight rolloff knee also moves earlier on hot scenes.
- Merged the v0.4.x engine into main: Instant Amount master knob (0-200%), preset-stomp fix with hidden `preset_applied` tracking, auto-detected `Custom (Manual Tuning)` preset state, Skin Beauty / Healthy Tone / 3D Depth Pop controls, symmetric 5/9-tap kernels, honest performance tiers (Lite = 1 tap), alpha-correct neighbor taps, wrap-aware Healthy Tone hue shift, GLSL-safe float3 constructors, missing-effect-path guard.
- Two-stage render path with automatic fallback to the classic single-pass path if texrender capture fails.
- Version bumped to 0.5.0 across buildspec, packaging script, and installer.

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
