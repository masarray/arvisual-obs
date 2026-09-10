# AGENTS.md — ArVisual OBS Production Engineering Contract

This file governs AI/code-agent work in ArVisual. ArVisual is a native OBS video filter with scene analysis and GPU rendering. Image integrity, OBS stability, bounded realtime cost, visual consistency, and regression safety are product requirements.

## Prime directive

Do not begin with a disposable or intentionally naive implementation when the production architecture is knowable. Prefer the smallest coherent production-quality change that preserves accepted visual behavior and performance.

Priority order:
1. no crash/black-frame/render corruption;
2. visual correctness and neutral/skin/highlight safety;
3. deterministic bounded realtime cost;
4. regression compatibility;
5. maintainability;
6. cosmetic convenience.

## Mandatory workflow

For non-trivial changes:

RECONNAISSANCE -> REPRODUCE/BASELINE -> ROOT CAUSE -> VISUAL/PERFORMANCE INVARIANTS -> ARCHITECTURE IMPACT -> IMPLEMENT -> REGRESSION TEST -> FAILURE TEST -> PERFORMANCE CHECK -> BUILD/CI -> DIRECT OBS VALIDATION.

Before editing, identify whether the issue belongs to scene analysis, adaptation/state, GPU shader/effect, preset/parameter handoff, render-resource lifecycle, or UI/configuration. Do not compensate for an analysis defect with a shader clamp or compensate for a shader defect with a preset hack.

If three successive patches in the same subsystem still treat symptoms, STOP before patch four and re-audit ownership, assumptions, and the demonstrated root cause.

## Architecture boundaries

Keep a single authoritative pipeline:

OBS frame/input
-> bounded scene analysis
-> validated/adapted parameters
-> GPU effect/render
-> output

Rules:
- UI/properties do not own image-analysis truth;
- presets configure the same engine; they do not create parallel DSP/render paths;
- performance tiers may change bounded analysis/render cost but must preserve explicit semantics;
- avoid duplicate scene statistics, hidden correction engines, or second shader state authorities;
- keep OBS/platform glue separated from analysis/math where practical.

## Exception-free render hot paths

Expected/recoverable failures must not use exceptions as routine control flow in per-frame analysis, render callbacks, shader parameter preparation, or high-frequency OBS callbacks.

Use compact explicit status/result contracts (`enum`, small status structs, `std::optional` when detail is unnecessary, or one project Result type where useful).

No exception may unwind through an OBS video/render callback. Framework/OS/filesystem/image/configuration exceptions may occur at non-realtime boundaries; catch them there and convert them to controlled state/diagnostics.

Do not globally disable exceptions merely to satisfy this rule. Use `noexcept` only where the complete reachable path is genuinely non-throwing.

## Bounded asynchronous diagnostics

Per-frame code must not perform file logging, network telemetry, JSON serialization, stack-trace generation, or repeated human-readable string formatting.

Hot paths may emit only tiny machine-readable events/counters into a bounded non-blocking diagnostic channel. Aggregate/deduplicate/rate-limit error storms. Queue saturation must have an explicit drop/coalesce policy.

Formatting/persistence/UI display belongs off the render path. Diagnostic failure must never stall rendering or destabilize OBS.

## GPU and shader safety

Every shader parameter used by the active draw path must receive a deterministic finite value before rendering. Never rely on stale parameter state.

Treat black output, NaN-driven shader behavior, device/resource misuse, flicker, and unexpected full-frame luminance/color shifts as P0/P1 regressions.

No per-frame shader compilation, texture creation/destruction, filesystem access, CPU readback, or other unbounded setup work.

Prepare expensive resources outside the steady-state render callback and publish coherent prepared state transactionally. Failed preparation retains last-known-good resources or safe pass-through.

## Numerical and visual safety

Defend explicitly against NaN/Inf/divide-by-zero, invalid dimensions, zero/near-zero denominators, invalid histogram/statistics counts, out-of-range parameters, and unsupported pixel/render conditions.

Clamps must encode a known visual/product bound, not hide an upstream bug.

Unless a task explicitly changes product behavior, preserve:
- neutral/white protection;
- highlight protection;
- bounded skin-specific correction;
- luminance-preserving gamut behavior;
- anti-halo clarity behavior;
- bounded scene adaptation;
- stable preset semantics;
- pass-through/safe fallback when processing cannot be trusted.

## Scene adaptation and state

Adaptive state must have one owner and explicit reset/reconfigure behavior. Avoid frame-history structures whose size grows with session duration.

Parameter or preset changes should use candidate -> validate/prepare -> coherent commit. Do not publish half-applied multi-parameter state across frames.

Analysis state from one source/frame geometry must not leak incorrectly after source resize, format change, device reset, or filter reinitialization.

## Realtime performance

The video path must remain bounded for 1080p60-class operation on representative supported GPUs/CPUs.

Avoid per-frame heap churn, unnecessary full-frame CPU copies, repeated invariant calculations, unnecessary analysis passes, and synchronous waits/locks.

Measure relevant changes, including where applicable:
- per-frame CPU analysis time;
- render/GPU pass count or timing;
- allocation rate;
- memory growth over long sessions;
- frame/render stalls;
- parameter-change response latency.

Performance tiers must reflect measured work reduction, not labels only. Do not claim optimization without evidence.

## Threading and lifecycle

OBS callbacks, graphics resources, property callbacks, timers/workers, and shared state require explicit ownership and shutdown/reconfigure paths.

Do not detach threads to avoid cleanup. Do not hold blocking locks across render callbacks. Use atomics, immutable snapshots, or bounded prepared-state handoff where appropriate.

Source/filter destroy or graphics reset must not leave callbacks pointing at retired resources.

## UI/property discipline

Properties configure and observe the engine; they do not run heavy scene analysis or become a second runtime state model.

Avoid property refresh loops, unnecessary rebuilds, or cosmetic animation that adds material OBS/UI load. Runtime labels must reflect actual active/prepared engine state rather than only the last requested setting.

## Testing and visual validation

Every defect fix should add/update a deterministic regression test where practical. Test the exact visual/numerical/runtime failure, not only a nearby happy path.

Automated tests should cover math, bounds, ABI/parameter contracts, preset semantics, and source-level regression risks. Direct OBS testing remains required for behavior automation cannot prove, including black-frame safety, flicker, GPU/device behavior, visual quality, and comfortable sustained rendering.

Use representative scenes: neutral/white surfaces, skin/talking head, highlights, dark scenes, highly saturated content, games/display capture, and stable/static frames.

## Change discipline

Do not:
- stack arbitrary clamps/thresholds after an unexplained visual defect;
- duplicate analysis/render state;
- add per-frame logging or allocation-heavy instrumentation;
- mix unrelated UI/release refactors into a render fix;
- upgrade OBS/dependencies incidentally;
- weaken quality/performance gates merely to make a patch pass.

Instrumentation for realtime defects must use bounded counters/snapshots and remain production-safe or be removed before completion.

## Definition of done

A task is not complete because CMake builds.

Validate as applicable:
QUALITY/DETERMINISTIC TESTS
+ REGRESSION TEST
+ NUMERICAL/FAILURE TEST
+ WINDOWS NATIVE/SHADER BUILD
+ PERFORMANCE/ALLOCATION CHECK
+ PACKAGE CONTRACT WHEN TOUCHED
+ DIRECT OBS VISUAL VALIDATION.

Never claim checks that were not actually run.

## Completion report

Report: changed, root cause, architecture/state decision, visual invariants preserved, Result/failure contract, regression protection, measured performance impact, exact validation, and remaining genuine limitations.

## Final rule

Think like the engineer responsible for every frame in a live OBS session. Keep analysis and render state coherent, keep the hot path bounded, fail safely, and require evidence before claiming a visual or performance improvement.
