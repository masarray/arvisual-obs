#!/usr/bin/env python3
"""Numerical regression for ArVisual's gamut-safe color core.

This is a dependency-free scalar port of the shader's tone, headroom-based
saturation, smart-risk, soft-shoulder, and gamut-mapping rules. It uses medians
measured from the user's unfiltered 24-patch ColorChecker screenshot and a dense
RGB cube. It is intentionally conservative: a failure means the shader's safety
contract changed and must be reviewed against a fresh OBS capture.
"""

from __future__ import annotations

import colorsys
import math
from pathlib import Path


LUMA = (0.2126, 0.7152, 0.0722)

# 4 columns x 6 rows, measured from the unfiltered screenshot supplied for the
# v0.5.3 clipping audit. Values are gamma-encoded RGB, matching the shader.
COLORCHECKER_8BIT = (
    (243, 243, 243), (56, 62, 150), (215, 126, 44), (115, 82, 67),
    (200, 200, 200), (70, 148, 73), (79, 91, 165), (195, 150, 131),
    (160, 160, 160), (175, 54, 61), (194, 90, 99), (98, 122, 160),
    (122, 122, 122), (231, 198, 31), (96, 60, 108), (87, 108, 67),
    (85, 85, 85), (186, 86, 148), (159, 187, 64), (134, 127, 178),
    (52, 52, 52), (8, 133, 161), (225, 162, 46), (105, 188, 169),
)


def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return min(max(value, low), high)


def smoothstep(edge0: float, edge1: float, value: float) -> float:
    t = clamp((value - edge0) / (edge1 - edge0))
    return t * t * (3.0 - 2.0 * t)


def luma(rgb: tuple[float, float, float]) -> float:
    return sum(channel * weight for channel, weight in zip(rgb, LUMA))


def saturation(rgb: tuple[float, float, float]) -> float:
    highest = max(rgb)
    return (highest - min(rgb)) / highest if highest > 1.0e-9 else 0.0


def hue_mask(hue: float, center: float, width: float) -> float:
    distance = abs(hue - center)
    distance = min(distance, 1.0 - distance)
    return clamp(1.0 - distance / width)


def gamut_map(rgb: tuple[float, float, float], target_luma: float) -> tuple[float, float, float]:
    """Match fit_gamut_preserve_luma() in data/effects/arvisual.effect."""
    target = clamp(target_luma)
    source_luma = luma(rgb)
    chroma = tuple(channel - source_luma for channel in rgb)
    low_bound = min(target, 0.008)
    high_bound = max(target, 0.992)
    scale = 1.0
    for delta in chroma:
        if delta > 0.0:
            scale = min(scale, (high_bound - target) / max(delta, 1.0e-6))
        elif delta < 0.0:
            scale = min(scale, (target - low_bound) / max(-delta, 1.0e-6))
    return tuple(clamp(target + delta * clamp(scale)) for delta in chroma)


def soft_shoulder(value: float, knee: float, pressure: float) -> float:
    over = max(value - knee, 0.0)
    rolled = value - over + over / (1.0 + over * (2.6 + pressure * 3.0))
    return min(rolled, 0.985)


def percentile(values: list[float], quantile: float) -> float:
    ordered = sorted(values)
    index = max(0, math.ceil(quantile * len(ordered)) - 1)
    return ordered[index]


def smart_controls(patches: tuple[tuple[float, float, float], ...]) -> dict[str, float]:
    active = [rgb for rgb in patches if luma(rgb) >= 0.020 or max(rgb) >= 0.035]
    luminances = [luma(rgb) for rgb in active]
    saturations = [saturation(rgb) for rgb in active if luma(rgb) >= 0.070]
    count = float(len(active))

    p10 = percentile(luminances, 0.10)
    p50 = percentile(luminances, 0.50)
    p90 = percentile(luminances, 0.90)
    p98 = percentile(luminances, 0.98)
    mean_sat = sum(saturations) / len(saturations)
    p90_sat = percentile(saturations, 0.90)
    shadow_frac = sum(luma(rgb) < 0.095 for rgb in active) / count
    near_clip_frac = sum(max(rgb) >= 0.975 for rgb in active) / count
    vivid_frac = sum(saturation(rgb) >= 0.78 for rgb in active) / count
    hot_vivid_frac = sum(saturation(rgb) >= 0.70 and max(rgb) >= 0.92 for rgb in active) / count
    neutral_frac = sum(luma(rgb) >= 0.15 and saturation(rgb) < 0.12 for rgb in active) / count
    colored_frac = sum(luma(rgb) >= 0.08 and saturation(rgb) > 0.35 for rgb in active) / count

    upper_key = p90 * 0.72 + p98 * 0.28
    exposure = clamp((0.64 - upper_key) * 0.09, -0.025, 0.012)
    p90_pressure = clamp((p90 - 0.80) / 0.16)
    p98_pressure = clamp((p98 - 0.91) / 0.075)
    clip_pressure = clamp((near_clip_frac - 0.012) * 10.0)
    hot_pressure = clamp((hot_vivid_frac - 0.008) * 12.0)
    highlight = max(p90_pressure * 0.70, p98_pressure, clip_pressure, hot_pressure)
    sat_pressure = clamp((mean_sat - 0.32) / 0.34)
    tail_sat_pressure = clamp((p90_sat - 0.68) / 0.28)
    vivid_pressure = clamp((vivid_frac - 0.16) / 0.40)
    color_risk = max(sat_pressure, tail_sat_pressure, vivid_pressure, hot_pressure)
    muted_lift = clamp((0.18 - mean_sat) * 0.16, 0.0, 0.025)
    pop = clamp(1.0 + muted_lift - color_risk * 0.48, 0.52, 1.025)
    strength = clamp(1.0 - color_risk * 0.34 - highlight * 0.16, 0.58, 1.0)
    chroma_limit = clamp(0.985 - color_risk * 0.060 - highlight * 0.025, 0.90, 0.985)
    neutral_dominance = clamp((neutral_frac - 0.30) / 0.45)
    colored_presence = clamp((colored_frac - 0.04) / 0.22)
    colorful_scene = clamp((mean_sat - 0.24) / 0.25)
    smart_clean = neutral_dominance * (1.0 - colorful_scene)
    smart_separation = neutral_dominance * colored_presence * (1.0 - highlight * 0.45)
    low_pressure = clamp((0.085 - p10) / 0.075)
    shadow_pressure = clamp((shadow_frac - 0.24) * 2.2)
    shadow = max(low_pressure, shadow_pressure)
    return {
        "exposure": exposure,
        "pop": pop,
        "highlight": highlight,
        "strength": strength,
        "chroma_limit": chroma_limit,
        "shadow": shadow,
        "p90_saturation": p90_sat,
        "clean": smart_clean,
        "separation": smart_separation,
    }


def grade_core(rgb: tuple[float, float, float], smart: dict[str, float]) -> tuple[float, float, float]:
    """Port the shader stages capable of changing global luma/chroma."""
    source_luma = luma(rgb)
    hue, source_sat, _ = colorsys.rgb_to_hsv(*rgb)
    strength = smart["strength"]
    enhance = 0.78 * strength
    pop = clamp(0.86 * strength * smart["pop"])
    depth = 0.76 * strength

    low_sat = 1.0 - smoothstep(0.030, 0.18, source_sat)
    neutral_luma = smoothstep(0.10, 0.36, source_luma) * (1.0 - smoothstep(0.965, 1.0, source_luma))
    neutral_mask = clamp(low_sat * neutral_luma)
    neutral_highlight = (1.0 - smoothstep(0.025, 0.12, source_sat)) * smoothstep(0.72, 0.92, source_luma)
    neutral_amount = neutral_mask * 0.72 * (0.62 + smart["clean"] * 0.18)
    cleaned = tuple(channel + (source_luma - channel) * neutral_amount for channel in rgb)
    green_cast = clamp((cleaned[1] - max(cleaned[0], cleaned[2])) * 4.0)
    cyan_cast = clamp((min(cleaned[1], cleaned[2]) - cleaned[0]) * 3.0)
    cast_guard = neutral_mask * 0.72 * (0.24 + smart["clean"] * 0.12) * clamp(green_cast + cyan_cast)
    cleaned_green = cleaned[1] + (((cleaned[0] + cleaned[2]) * 0.5) - cleaned[1]) * cast_guard
    cleaned = gamut_map((cleaned[0], cleaned_green, cleaned[2]), luma((cleaned[0], cleaned_green, cleaned[2])))
    tone_luma = luma(cleaned)

    skin_hue = max(hue_mask(hue, 0.055, 0.060), hue_mask(hue, 0.092, 0.070), hue_mask(hue, 0.125, 0.050))
    skin_luma = smoothstep(0.070, 0.26, source_luma) * (1.0 - smoothstep(0.90, 1.0, source_luma))
    skin_sat = smoothstep(0.055, 0.28, source_sat) * (1.0 - smoothstep(0.86, 1.0, source_sat))
    brown_skin = (
        smoothstep(0.08, 0.38, source_luma)
        * smoothstep(0.16, 0.52, source_sat)
        * hue_mask(hue, 0.085, 0.085)
    )
    skin_mask = clamp(skin_hue * skin_luma * skin_sat + brown_skin * 0.45)

    mid = smoothstep(0.12, 0.38, tone_luma) * (1.0 - smoothstep(0.76, 0.94, tone_luma))
    upper_mid = smoothstep(0.34, 0.62, tone_luma) * (1.0 - smoothstep(0.84, 0.98, tone_luma))
    deep_shadow = 1.0 - smoothstep(0.035, 0.18, tone_luma)
    highlight = smoothstep(0.72, 0.98, tone_luma)
    vivid_bright = smoothstep(0.48, 0.78, source_sat) * smoothstep(0.50, 0.82, tone_luma)
    lift_scale = 1.0 - vivid_bright * 0.90
    expose_band = smoothstep(0.06, 0.23, tone_luma) * (1.0 - smoothstep(0.68, 0.90, tone_luma))

    target_luma = tone_luma
    target_luma += smart["exposure"] * expose_band
    target_luma += enhance * 0.032 * mid * lift_scale
    target_luma += enhance * depth * 0.022 * upper_mid * lift_scale
    target_luma -= depth * 0.010 * deep_shadow * (1.0 - smart["shadow"] * 0.78)
    target_luma += smart["shadow"] * 0.007 * deep_shadow
    diffuse_white_guard = 1.0 - neutral_highlight * 0.88
    target_luma -= (0.94 * 0.006 + smart["highlight"] * 0.008) * highlight * diffuse_white_guard
    tone_delta = target_luma - tone_luma
    channel_lift_room = 1.0 - smoothstep(0.82, 0.985, max(cleaned))
    skin_lift_guard = 1.0 - skin_mask * 0.42
    neutral_lift_guard = 1.0 - neutral_mask * (0.35 + smart["clean"] * 0.15)
    target_luma = (
        tone_luma
        + min(tone_delta, 0.0)
        + max(tone_delta, 0.0) * channel_lift_room * skin_lift_guard * neutral_lift_guard
    )
    contrast = (enhance * 0.042 + depth * 0.030) * (1.0 - vivid_bright * 0.75)
    contrast_target = (target_luma - 0.5) * (1.0 + contrast) + 0.5
    contrast_band = smoothstep(0.075, 0.24, target_luma) * (1.0 - smoothstep(0.80, 0.94, target_luma))
    target_luma += (contrast_target - target_luma) * contrast_band * neutral_lift_guard
    tone_knee = (0.84 - smart["highlight"] * 0.035) + (
        0.972 - (0.84 - smart["highlight"] * 0.035)
    ) * neutral_highlight
    target_luma = soft_shoulder(target_luma, tone_knee, smart["highlight"])
    toned = gamut_map(cleaned, target_luma)

    split_luma = luma(toned)
    shadow_chroma_confidence = smoothstep(0.040, 0.16, source_sat)
    cool_shadow = (
        (1.0 - smoothstep(0.18, 0.44, split_luma))
        * (1.0 - neutral_mask * 0.90)
        * shadow_chroma_confidence
    )
    warm_light = (
        smoothstep(0.42, 0.76, split_luma)
        * (1.0 - smoothstep(0.90, 1.0, split_luma))
        * (1.0 - neutral_mask * 0.96)
    )
    split = (
        toned[0] + depth * 0.006 * cool_shadow * -0.10 + depth * 0.005 * warm_light * 0.14,
        toned[1] + depth * 0.006 * cool_shadow * 0.01 + depth * 0.005 * warm_light * 0.035,
        toned[2] + depth * 0.006 * cool_shadow * 0.15 + depth * 0.005 * warm_light * -0.05,
    )
    toned = gamut_map(split, luma(split))

    toned_luma = luma(toned)
    current_hue, current_sat, current_value = colorsys.rgb_to_hsv(*toned)
    color_confidence = smoothstep(0.10, 0.34, source_sat) * smoothstep(0.065, 0.20, source_luma) * (
        1.0 - smoothstep(0.95, 1.0, source_luma)
    )
    luma_safe = smoothstep(0.055, 0.20, toned_luma) * (1.0 - smoothstep(0.86, 0.98, toned_luma))
    muted_colored = smoothstep(0.045, 0.18, current_sat) * (1.0 - smoothstep(0.84, 0.98, current_sat))
    vibrance = (
        pop
        * muted_colored
        * color_confidence
        * luma_safe
        * (1.0 - neutral_mask * 0.98)
        * (1.0 - skin_mask * 0.90)
    )
    dopamine = clamp(
        hue_mask(current_hue, 0.000, 0.050)
        + hue_mask(current_hue, 0.080, 0.055) * 0.75
        + hue_mask(current_hue, 0.155, 0.052) * 0.78
        + hue_mask(current_hue, 0.330, 0.055) * 0.62
        + hue_mask(current_hue, 0.500, 0.050) * 0.55
        + hue_mask(current_hue, 0.620, 0.064)
        + hue_mask(current_hue, 0.835, 0.050) * 0.72
    )
    object_pop = dopamine * color_confidence * (1.0 - neutral_mask) * (1.0 - skin_mask * 0.88)
    sat_ceiling = max(current_sat, smart["chroma_limit"])
    sat_room = max(sat_ceiling - current_sat, 0.0)
    sat_request = clamp(vibrance * 0.38 + object_pop * pop * (0.18 + depth * 0.04))
    blue_safety = hue_mask(current_hue, 0.600, 0.120)
    blue_sat_guard = 1.0 - blue_safety * (0.34 + smart["separation"] * 0.28)
    sat_request *= blue_sat_guard
    max_sat_gain = 0.075 + (0.110 - 0.075) * smart["strength"]
    green_teal_safety = clamp(
        hue_mask(current_hue, 0.300, 0.160) * 1.35 + hue_mask(current_hue, 0.500, 0.140)
    )
    violet_safety = clamp(hue_mask(current_hue, 0.730, 0.140) * 1.10)
    secondary_safety = clamp(green_teal_safety + violet_safety + blue_safety * 0.80)
    scene_chroma_risk = clamp((0.985 - smart["chroma_limit"]) / 0.085)
    secondary_gain_cap = 0.058 + (0.046 - 0.058) * scene_chroma_risk
    max_sat_gain += (secondary_gain_cap - max_sat_gain) * secondary_safety
    output_sat = current_sat + min(sat_room * sat_request, max_sat_gain)
    saturated = colorsys.hsv_to_rgb(current_hue, clamp(output_sat), current_value)
    output = gamut_map(saturated, toned_luma)

    output_luma = luma(output)
    warm_hero = clamp(
        hue_mask(current_hue, 0.000, 0.050) * 0.55
        + hue_mask(current_hue, 0.080, 0.055)
        + hue_mask(current_hue, 0.155, 0.052) * 0.35
        + hue_mask(current_hue, 0.835, 0.050) * 0.18
    )
    fresh_hero = clamp(hue_mask(current_hue, 0.500, 0.050) * 0.55 + hue_mask(current_hue, 0.330, 0.055) * 0.30)
    blue_hero = hue_mask(current_hue, 0.620, 0.064) * (1.0 - hue_mask(current_hue, 0.500, 0.050) * 0.65)
    hero_band = smoothstep(0.10, 0.28, output_luma) * (1.0 - smoothstep(0.72, 0.90, output_luma))
    warm_separation = 0.012 + smart["separation"] * 0.012
    fresh_separation = 0.005 + smart["separation"] * 0.004
    blue_density = 0.005 + smart["separation"] * 0.005
    hero_delta = object_pop * pop * hero_band * (
        warm_hero * warm_separation + fresh_hero * fresh_separation - blue_hero * blue_density
    )
    final_knee_base = 0.84 - smart["highlight"] * 0.045 - 0.94 * 0.010
    final_knee = final_knee_base + (0.972 - final_knee_base) * neutral_highlight
    final_luma = soft_shoulder(output_luma + hero_delta, final_knee, smart["highlight"] + 0.94 * 0.35)
    hero_output = gamut_map(output, final_luma)
    _, hero_sat_after, hero_value_after = colorsys.rgb_to_hsv(*hero_output)
    synthetic_warm_object = color_confidence * smoothstep(0.68, 0.82, source_sat) * (1.0 - neutral_mask)
    warm_chroma_object = max(object_pop, synthetic_warm_object)
    warm_chroma_retention = clamp(warm_hero * warm_chroma_object * (1.25 + smart["separation"] * 0.35))
    warm_sat_floor = min(source_sat, smart["chroma_limit"])
    retained_sat = hero_sat_after + max(warm_sat_floor - hero_sat_after, 0.0) * warm_chroma_retention
    retained = colorsys.hsv_to_rgb(current_hue, clamp(retained_sat), hero_value_after)
    return gamut_map(retained, luma(hero_output))


def main() -> None:
    shader_text = (
        Path(__file__).resolve().parents[1] / "data" / "effects" / "arvisual.effect"
    ).read_text(encoding="utf-8")
    assert "fit_gamut_preserve_luma(color, y + clarity_delta)" in shader_text
    assert "soft_luma_shoulder(y + clarity_delta, 0.84" not in shader_text

    patches = tuple(tuple(channel / 255.0 for channel in patch) for patch in COLORCHECKER_8BIT)
    smart = smart_controls(patches)
    outputs = tuple(grade_core(patch, smart) for patch in patches)

    for rgb in outputs:
        assert all(math.isfinite(channel) for channel in rgb), f"non-finite output: {rgb}"
        assert all(0.0 <= channel <= 1.0 for channel in rgb), f"out of gamut: {rgb}"

    # Saturation enhancement may approach the adaptive ceiling but cannot
    # exceed an already-more-saturated input.
    for source, output in zip(patches, outputs):
        permitted = max(saturation(source), smart["chroma_limit"])
        assert saturation(output) <= permitted + 2.0e-6, (source, output, permitted)
        assert saturation(output) - saturation(source) <= 0.112, (source, output)

    # Neutral chart patches must stay neutral; they must not acquire a cast.
    for index in (0, 4, 8, 12, 16, 20):
        assert saturation(outputs[index]) <= 0.012, (index, outputs[index])

    # Diffuse white retains clean sparkle rather than being rolled into gray.
    assert luma(outputs[0]) >= luma(patches[0]) - 0.008, (patches[0], outputs[0])

    # Flat secondary colors receive a risk-aware budget rather than the same
    # saturation allowance as warm hero colors.
    for index in (5, 6, 11, 14, 15, 19, 23):
        assert saturation(outputs[index]) - saturation(patches[index]) <= 0.058, (
            index,
            patches[index],
            outputs[index],
        )

    # Neutral midtones get dimensional contrast without a clinical lift.
    assert luma(outputs[4]) - luma(patches[4]) <= 0.026, (patches[4], outputs[4])

    # No visible input patch may collapse to black or gain a new hard clip.
    for source, output in zip(patches, outputs):
        if luma(source) >= 0.08:
            assert luma(output) >= 0.055, (source, output)
        if max(source) < 0.95:
            assert max(output) < 0.999, (source, output)
        assert abs(luma(output) - luma(source)) <= 0.075, (source, output)

        if saturation(source) >= 0.08 and saturation(output) >= 0.08:
            source_hue = colorsys.rgb_to_hsv(*source)[0]
            output_hue = colorsys.rgb_to_hsv(*output)[0]
            hue_delta = abs(((output_hue - source_hue + 0.5) % 1.0) - 0.5) * 360.0
            assert hue_delta <= 1.0, (source, output, hue_delta)

    # Deep low-key detail is protected from the midtone contrast pivot.
    toe_input = (0.05, 0.05, 0.05)
    toe_output = grade_core(toe_input, smart)
    assert 0.035 <= luma(toe_output) <= 0.065, toe_output

    # Simulate the audited low-key two-panel scene: a large dark background,
    # healthy upper-key content, and a small vivid color tail. The upper-key
    # exposure model must preserve mood while P90 saturation still trims dose.
    low_key_scene = (
        ((0.04, 0.04, 0.04),) * 46
        + ((0.10, 0.10, 0.10),) * 10
        + ((0.30, 0.30, 0.30),) * 10
        + ((0.55, 0.55, 0.55),) * 14
        + ((0.78, 0.78, 0.78),) * 8
        + ((0.08, 0.55, 0.12),) * 12
    )
    low_key_smart = smart_controls(low_key_scene)
    assert low_key_smart["exposure"] <= 0.006, low_key_smart
    assert low_key_smart["strength"] < 0.92, low_key_smart

    # Real-video product/tutorial model measured from the Fluke before frame:
    # dominant cool-neutral workbench, sparse orange/red/blue objects, natural
    # brown skin, dense product blacks, and a small highlight tail.
    rgb8 = lambda triplet: tuple(channel / 255.0 for channel in triplet)
    product_samples = {
        "table": rgb8((168, 174, 170)),
        "white": rgb8((189, 196, 191)),
        "orange": rgb8((190, 126, 34)),
        "red": rgb8((179, 53, 51)),
        "blue": rgb8((49, 98, 138)),
        "skin": rgb8((136, 93, 66)),
        "dark": rgb8((40, 43, 42)),
    }
    product_scene = (
        (product_samples["table"],) * 58
        + (product_samples["white"],) * 8
        + ((0.92, 0.92, 0.92),) * 4
        + (product_samples["orange"],) * 8
        + (product_samples["red"],) * 3
        + (product_samples["blue"],) * 3
        + (product_samples["skin"],) * 8
        + (product_samples["dark"],) * 8
    )
    product_scene_smart = smart_controls(product_scene)
    # Controls measured from the full 64x36 downsample of the aligned real
    # product frame, rather than inferred from only the anchor colors above.
    product_smart = {
        **product_scene_smart,
        "exposure": -0.0121,
        "pop": 1.0,
        "highlight": 0.0,
        "strength": 1.0,
        "chroma_limit": 0.985,
        "shadow": 0.0,
        "clean": 0.768,
        "separation": 0.707,
    }
    product_outputs = {name: grade_core(rgb, product_smart) for name, rgb in product_samples.items()}

    assert product_scene_smart["exposure"] < -0.008, product_scene_smart
    assert product_scene_smart["clean"] > 0.70, product_scene_smart
    assert product_scene_smart["separation"] > 0.45, product_scene_smart

    table_in, table_out = product_samples["table"], product_outputs["table"]
    assert saturation(table_out) <= saturation(table_in) * 0.70, (table_in, table_out)
    assert 0.008 <= luma(table_out) - luma(table_in) <= 0.026, (table_in, table_out)

    white_in, white_out = product_samples["white"], product_outputs["white"]
    assert 0.010 <= luma(white_out) - luma(white_in) <= 0.032, (white_in, white_out)

    orange_in, orange_out = product_samples["orange"], product_outputs["orange"]
    assert 0.005 <= luma(orange_out) - luma(orange_in) <= 0.045, (orange_in, orange_out)
    assert saturation(orange_out) >= saturation(orange_in) - 0.006, (orange_in, orange_out)

    skin_in, skin_out = product_samples["skin"], product_outputs["skin"]
    assert abs(luma(skin_out) - luma(skin_in)) <= 0.020, (skin_in, skin_out)
    assert abs(saturation(skin_out) - saturation(skin_in)) <= 0.020, (skin_in, skin_out)

    blue_in, blue_out = product_samples["blue"], product_outputs["blue"]
    assert luma(blue_out) - luma(blue_in) <= 0.012, (blue_in, blue_out)
    assert saturation(blue_out) >= saturation(blue_in), (blue_in, blue_out)
    assert saturation(blue_out) - saturation(blue_in) <= 0.045, (blue_in, blue_out)

    dark_in, dark_out = product_samples["dark"], product_outputs["dark"]
    assert luma(dark_out) >= luma(dark_in) - 0.040, (dark_in, dark_out)
    assert saturation(dark_out) <= saturation(dark_in) + 0.006, (dark_in, dark_out)

    # A truly neutral shadow must never acquire the cool depth split.
    neutral_dark = rgb8((52, 52, 52))
    neutral_dark_out = grade_core(neutral_dark, product_smart)
    assert saturation(neutral_dark_out) <= 0.002, neutral_dark_out

    # Dense cube: mathematical gamut/finite invariant, including extreme RGB.
    for ri in range(17):
        for gi in range(17):
            for bi in range(17):
                source = (ri / 16.0, gi / 16.0, bi / 16.0)
                output = grade_core(source, smart)
                assert all(math.isfinite(channel) and 0.0 <= channel <= 1.0 for channel in output), (
                    source,
                    output,
                )
                if 0 < ri < 16 and 0 < gi < 16 and 0 < bi < 16:
                    assert min(output) >= 0.0079 and max(output) <= 0.9921, (source, output)

    mean_sat_gain = sum(saturation(out) - saturation(src) for src, out in zip(patches, outputs)) / len(patches)
    max_luma_delta = max(abs(luma(out) - luma(src)) for src, out in zip(patches, outputs))
    print("ArVisual color-safety regression: PASS")
    print(
        "Smart controls: "
        f"pop={smart['pop']:.3f}, strength={smart['strength']:.3f}, "
        f"highlight={smart['highlight']:.3f}, chroma_limit={smart['chroma_limit']:.3f}"
    )
    print(f"24-patch mean saturation delta: {mean_sat_gain:+.4f}")
    print(f"24-patch maximum luma delta: {max_luma_delta:.4f}")
    print(
        "Product-scene controls: "
        f"clean={product_smart['clean']:.3f}, separation={product_smart['separation']:.3f}, "
        f"exposure={product_smart['exposure']:+.4f}"
    )
    print(
        "Product anchors dY: "
        f"orange={luma(product_outputs['orange']) - luma(product_samples['orange']):+.4f}, "
        f"skin={luma(product_outputs['skin']) - luma(product_samples['skin']):+.4f}, "
        f"blue={luma(product_outputs['blue']) - luma(product_samples['blue']):+.4f}"
    )
    print("Dense RGB cube checked: 4913 colors")


if __name__ == "__main__":
    main()
