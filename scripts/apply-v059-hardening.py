#!/usr/bin/env python3
"""One-time migration used to prepare the v0.5.9 hardening release branch."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.5.9"
TEMPLATE_REF = "3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc"


def path(relative: str) -> Path:
    return ROOT / relative


def read(relative: str) -> str:
    return path(relative).read_text(encoding="utf-8")


def write(relative: str, content: str) -> None:
    path(relative).write_text(content, encoding="utf-8")


def replace_once(relative: str, old: str, new: str) -> None:
    text = read(relative)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"Expected one occurrence in {relative}, found {count}: {old[:100]!r}")
    write(relative, text.replace(old, new, 1))


def regex_once(relative: str, pattern: str, replacement: str, flags: int = 0) -> None:
    text = read(relative)
    updated, count = re.subn(pattern, replacement, text, count=1, flags=flags)
    if count != 1:
        raise RuntimeError(f"Expected one regex match in {relative}, found {count}: {pattern!r}")
    write(relative, updated)


# Release identity.
buildspec = json.loads(read("buildspec.json"))
buildspec["version"] = VERSION
write("buildspec.json", json.dumps(buildspec, indent=2) + "\n")

replace_once("CMakeLists.txt", "project(arvisual VERSION 0.5.8 LANGUAGES C CXX)", f"project(arvisual VERSION {VERSION} LANGUAGES C CXX)")
replace_once("packaging/windows/arvisual.iss", '#define AppVersion "0.5.8"', f'#define AppVersion "{VERSION}"')
replace_once("scripts/package-windows.ps1", '[string] $Version = "0.5.8"', f'[string] $Version = "{VERSION}"')
replace_once("data/effects/arvisual.effect", "v0.5.7 scene-intelligent professional engine:", f"v{VERSION} scene-intelligent professional engine:")
replace_once("docs/index.html", '"softwareVersion": "0.5.8"', f'"softwareVersion": "{VERSION}"')
replace_once("docs/index.html", "data-release-version>v0.5.8<", f"data-release-version>v{VERSION}<")
replace_once(".github/ISSUE_TEMPLATE/bug_report.yml", "placeholder: v0.5.8", f"placeholder: v{VERSION}")
replace_once("README.md", "ArVisual v0.5.8 uses", f"ArVisual v{VERSION} uses")
replace_once("README.md", "git tag v0.5.8\ngit push origin v0.5.8", f"git tag v{VERSION}\ngit push origin v{VERSION}")

# Pin the external template to a reviewed immutable commit and make cached local builds deterministic.
replace_once("scripts/build-local-windows.ps1", "[string] $TemplateRef = 'master'", f"[string] $TemplateRef = '{TEMPLATE_REF}'")
replace_once(
    "scripts/build-local-windows.ps1",
    """if (-not (Test-Path $TemplateDir)) {
    Write-Host 'Cloning official OBS plugin template...'
    Invoke-Logged git @('clone', '--depth', '1', '--branch', $TemplateRef, 'https://github.com/obsproject/obs-plugintemplate.git', $TemplateDir) $BuildRoot
} elseif (-not $Ci) {
    Write-Host 'Updating official OBS plugin template...'
    Invoke-Logged git @('fetch', '--depth', '1', 'origin', $TemplateRef) $TemplateDir
    Invoke-Logged git @('checkout', 'FETCH_HEAD') $TemplateDir
}
""",
    """if (-not (Test-Path $TemplateDir)) {
    Write-Host 'Creating the OBS plugin template workspace...'
    Invoke-Logged git @('clone', '--filter=blob:none', '--no-checkout', 'https://github.com/obsproject/obs-plugintemplate.git', $TemplateDir) $BuildRoot
}

Write-Host "Resolving pinned OBS plugin template commit $TemplateRef..."
Invoke-Logged git @('fetch', '--depth', '1', 'origin', $TemplateRef) $TemplateDir
Invoke-Logged git @('checkout', '--detach', '--force', 'FETCH_HEAD') $TemplateDir
Invoke-Logged git @('clean', '-ffd') $TemplateDir
""",
)
replace_once(".github/workflows/release.yml", "  TEMPLATE_REF: master", f"  TEMPLATE_REF: {TEMPLATE_REF}")

# Native object lifetime, GPU-resource fallback, diagnostics, and ABI enforcement.
replace_once("src/arvisual-filter.cpp", "#include <cstring>\n", "#include <cstring>\n#include <new>\n")
replace_once(
    "src/arvisual-filter.cpp",
    "    bool effect_compatible = false;\n\n    /* Smart engine graphics resources. */",
    "    bool effect_compatible = false;\n    bool smart_resources_ready = false;\n\n    /* Smart engine graphics resources. */",
)
replace_once(
    "src/arvisual-filter.cpp",
    """void destroy(void *data)
{
    auto *filter = static_cast<ArVisualFilter *>(data);

    obs_enter_graphics();
    if (filter->effect)
        gs_effect_destroy(filter->effect);
    if (filter->input_tr)
        gs_texrender_destroy(filter->input_tr);
    if (filter->analysis_tr)
        gs_texrender_destroy(filter->analysis_tr);
    for (int i = 0; i < 2; i++) {
        if (filter->stage[i])
            gs_stagesurface_destroy(filter->stage[i]);
    }
    obs_leave_graphics();

    bfree(filter);
}
""",
    """void destroy(void *data)
{
    auto *filter = static_cast<ArVisualFilter *>(data);
    if (!filter)
        return;

    obs_enter_graphics();
    if (filter->effect)
        gs_effect_destroy(filter->effect);
    if (filter->input_tr)
        gs_texrender_destroy(filter->input_tr);
    if (filter->analysis_tr)
        gs_texrender_destroy(filter->analysis_tr);
    for (int i = 0; i < 2; i++) {
        if (filter->stage[i])
            gs_stagesurface_destroy(filter->stage[i]);
    }
    obs_leave_graphics();

    delete filter;
}
""",
)
replace_once(
    "src/arvisual-filter.cpp",
    """void *create(obs_data_t *settings, obs_source_t *context)
{
    auto *filter = static_cast<ArVisualFilter *>(bzalloc(sizeof(ArVisualFilter)));
    filter->context = context;
    filter->smart_pop = 1.0f;
    filter->smart_strength = 1.0f;
    filter->smart_chroma_limit = 0.985f;
    filter->stats = SceneStats();
""",
    """void *create(obs_data_t *settings, obs_source_t *context)
{
    auto *filter = new (std::nothrow) ArVisualFilter{};
    if (!filter) {
        blog(LOG_ERROR, "[ArVisual] Failed to allocate filter state");
        return nullptr;
    }

    filter->context = context;
""",
)
replace_once("src/arvisual-filter.cpp", "        bfree(filter);\n        return nullptr;", "        delete filter;\n        return nullptr;")
replace_once(
    "src/arvisual-filter.cpp",
    "    obs_enter_graphics();\n    filter->effect = gs_effect_create_from_file(effect_path, nullptr);",
    "    char *effect_errors = nullptr;\n\n    obs_enter_graphics();\n    filter->effect = gs_effect_create_from_file(effect_path, &effect_errors);",
)
replace_once(
    "src/arvisual-filter.cpp",
    """        if (filter->effect_compatible) {
            filter->input_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->analysis_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->stage[0] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
            filter->stage[1] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
        }
""",
    """        if (filter->effect_compatible) {
            filter->input_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->analysis_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->stage[0] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
            filter->stage[1] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
            filter->smart_resources_ready = filter->input_tr && filter->analysis_tr && filter->stage[0] && filter->stage[1];
        }
""",
)
replace_once(
    "src/arvisual-filter.cpp",
    """    obs_leave_graphics();

    bfree(effect_path);
""",
    """    obs_leave_graphics();

    if (effect_errors) {
        blog(filter->effect ? LOG_WARNING : LOG_ERROR, "[ArVisual] Effect compiler: %s", effect_errors);
        bfree(effect_errors);
    }

    if (filter->effect_compatible && !filter->smart_resources_ready) {
        blog(LOG_WARNING,
             "[ArVisual] Smart Auto analysis resources could not be created. "
             "The normal color grade remains active without scene analysis.");
    }

    bfree(effect_path);
""",
)
replace_once(
    "src/arvisual-filter.cpp",
    """void read_scene_stats(ArVisualFilter *filter, float dt)
{
    const int read_idx = 1 - filter->stage_write;
""",
    """void read_scene_stats(ArVisualFilter *filter, float dt)
{
    if (!filter->smart_resources_ready)
        return;

    const int read_idx = 1 - filter->stage_write;
""",
)
replace_once("src/arvisual-filter.cpp", "    if (filter->smart)\n        read_scene_stats(filter, dt);", "    if (filter->smart && filter->smart_resources_ready)\n        read_scene_stats(filter, dt);")
replace_once(
    "src/arvisual-filter.cpp",
    """    bool captured = false;
    gs_texrender_reset(filter->input_tr);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
    if (filter->input_tr && gs_texrender_begin(filter->input_tr, width, height)) {
        vec4 clear_color;
        vec4_zero(&clear_color);
        gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
        gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
        obs_source_process_filter_end(filter->context, obs_get_base_effect(OBS_EFFECT_DEFAULT), width, height);
        gs_texrender_end(filter->input_tr);
        captured = true;
    }
    gs_blend_state_pop();
""",
    """    bool captured = false;
    if (filter->input_tr) {
        gs_texrender_reset(filter->input_tr);
        gs_blend_state_push();
        gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
        if (gs_texrender_begin(filter->input_tr, width, height)) {
            vec4 clear_color;
            vec4_zero(&clear_color);
            gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
            gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
            obs_source_process_filter_end(filter->context, obs_get_base_effect(OBS_EFFECT_DEFAULT), width, height);
            gs_texrender_end(filter->input_tr);
            captured = true;
        }
        gs_blend_state_pop();
    }
""",
)
replace_once(
    "src/arvisual-filter.cpp",
    "    if (filter->smart && input_tex && filter->analysis_tr) {",
    "    if (filter->smart && filter->smart_resources_ready && input_tex) {",
)

replace_once(
    "data/effects/arvisual.effect",
    """    float4 px = image.Sample(textureSampler, vert_in.uv);
    float alpha = px.a;
""",
    """    float4 px = image.Sample(textureSampler, vert_in.uv);
    if (abs(arvisual_shader_abi_v3 - 3.0) > 0.001)
        return px;

    float alpha = px.a;
""",
)

replace_once(
    "scripts/validate-color-safety.py",
    '    assert "fit_gamut_preserve_luma(color, y + clarity_delta)" in shader_text\n',
    '    assert "if (abs(arvisual_shader_abi_v3 - 3.0) > 0.001)" in shader_text\n'
    '    assert "fit_gamut_preserve_luma(color, y + clarity_delta)" in shader_text\n',
)

# Landing-page social sharing uses a raster card for reliable previews.
replace_once("docs/index.html", "assets/product-preview.svg", "assets/social-preview.png")

social_svg = """<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="630" viewBox="0 0 1200 630" role="img" aria-labelledby="title desc">
  <title id="title">ArVisual Smart Color Enhancer for OBS</title>
  <desc id="desc">Scene-adaptive color enhancement with protected skin tones, clean whites and controlled highlights.</desc>
  <defs>
    <linearGradient id="bg" x1="70" y1="30" x2="1120" y2="610" gradientUnits="userSpaceOnUse">
      <stop stop-color="#0b1020"/><stop offset="0.55" stop-color="#121a31"/><stop offset="1" stop-color="#0a1020"/>
    </linearGradient>
    <linearGradient id="g1" x1="82" y1="82" x2="264" y2="264" gradientUnits="userSpaceOnUse">
      <stop stop-color="#61e7ff"/><stop offset="0.42" stop-color="#6f7cff"/><stop offset="0.72" stop-color="#bf5cff"/><stop offset="1" stop-color="#ff5f8f"/>
    </linearGradient>
    <linearGradient id="g2" x1="252" y1="82" x2="96" y2="278" gradientUnits="userSpaceOnUse">
      <stop stop-color="#fff36e"/><stop offset="0.38" stop-color="#ff9d4d"/><stop offset="1" stop-color="#ff5474"/>
    </linearGradient>
    <linearGradient id="accent" x1="365" y1="420" x2="1050" y2="420" gradientUnits="userSpaceOnUse">
      <stop stop-color="#61e7ff"/><stop offset="0.48" stop-color="#8c70ff"/><stop offset="1" stop-color="#ff6d88"/>
    </linearGradient>
    <filter id="blur"><feGaussianBlur stdDeviation="48"/></filter>
  </defs>
  <rect width="1200" height="630" rx="38" fill="url(#bg)"/>
  <circle cx="1030" cy="80" r="210" fill="#5547d8" opacity="0.18" filter="url(#blur)"/>
  <circle cx="1040" cy="565" r="190" fill="#ff5f8f" opacity="0.12" filter="url(#blur)"/>
  <g transform="translate(72 135) scale(2.05)">
    <path d="M64 8c20.7 0 39.2 11.1 49.2 28.2L91.7 48.6A32 32 0 0 0 64 32V8Z" fill="url(#g1)"/>
    <path d="M113.2 36.2A56 56 0 0 1 112 94H87.1A32 32 0 0 0 91.7 48.6l21.5-12.4Z" fill="url(#g2)"/>
    <path d="M112 94a56 56 0 0 1-50 26V96a32 32 0 0 0 25.1-2H112Z" fill="url(#g1)"/>
    <path d="M62 120A56 56 0 0 1 14.8 91.8l21.5-12.4A32 32 0 0 0 62 96v24Z" fill="url(#g2)"/>
    <path d="M14.8 91.8A56 56 0 0 1 16 34h24.9a32 32 0 0 0-4.6 45.4L14.8 91.8Z" fill="url(#g1)"/>
    <path d="M16 34A56 56 0 0 1 64 8v24a32 32 0 0 0-23.1 2H16Z" fill="url(#g2)"/>
    <circle cx="64" cy="64" r="17" fill="#0d1324"/><circle cx="64" cy="64" r="8" fill="url(#g1)"/>
  </g>
  <text x="365" y="205" fill="#f7f9ff" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="72" font-weight="600" letter-spacing="-2">ArVisual</text>
  <text x="368" y="264" fill="#aeb8d2" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="27" font-weight="400">SMART COLOR ENHANCER FOR OBS</text>
  <text x="368" y="356" fill="#ffffff" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="39" font-weight="500">Cleaner color. Protected skin.</text>
  <text x="368" y="405" fill="#ffffff" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="39" font-weight="500">Controlled highlights.</text>
  <rect x="368" y="459" width="680" height="5" rx="2.5" fill="url(#accent)"/>
  <g fill="#cbd3e8" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="24">
    <text x="368" y="520">Scene adaptive</text><text x="585" y="520">Native OBS filter</text><text x="835" y="520">Open source</text>
  </g>
  <text x="368" y="574" fill="#7986a5" font-family="Inter,Segoe UI,Arial,sans-serif" font-size="20">masarray.github.io/arvisual-obs</text>
</svg>
"""
write("docs/assets/social-preview.svg", social_svg)

# Expand repository contracts for the new release and runtime safeguards.
validator = read("scripts/validate-repository.py")
validator = validator.replace(
    """    docs_fallback_version = capture(
        r'data-release-version>v([0-9.]+)<',
        read("docs/index.html"),
        "landing-page release fallback version",
    )
""",
    """    docs_fallback_version = capture(
        r'data-release-version>v([0-9.]+)<',
        read("docs/index.html"),
        "landing-page release fallback version",
    )
    local_template_ref = capture(
        r"\\[string\\]\\s+\\$TemplateRef\\s*=\\s*'([0-9a-f]{40})'",
        read("scripts/build-local-windows.ps1"),
        "local OBS template commit",
    )
    release_template_ref = capture(
        r"TEMPLATE_REF:\\s*([0-9a-f]{40})",
        read(".github/workflows/release.yml"),
        "release OBS template commit",
    )
""",
    1,
)
validator = validator.replace(
    "    require(not mismatches, f\"Version drift detected; expected {version}: {mismatches}\")\n",
    "    require(not mismatches, f\"Version drift detected; expected {version}: {mismatches}\")\n"
    "    require(local_template_ref == release_template_ref, \"Local and release OBS template commits differ\")\n",
    1,
)
validator = validator.replace(
    """    require(english_keys == indonesian_keys, f"Locale key mismatch: en-only={english_keys - indonesian_keys}, id-only={indonesian_keys - english_keys}")

    installer = read("packaging/windows/arvisual.iss")
""",
    """    require(english_keys == indonesian_keys, f"Locale key mismatch: en-only={english_keys - indonesian_keys}, id-only={indonesian_keys - english_keys}")

    require((ROOT / "docs/assets/social-preview.png").is_file(), "Missing raster social preview")
    landing_page = read("docs/index.html")
    require(landing_page.count("assets/social-preview.png") >= 2, "Open Graph and Twitter cards must use the PNG preview")

    source = read("src/arvisual-filter.cpp")
    for marker in (
        "new (std::nothrow) ArVisualFilter{}",
        "smart_resources_ready",
        "gs_effect_create_from_file(effect_path, &effect_errors)",
    ):
        require(marker in source, f"Native runtime hardening is missing {marker!r}")
    require("bzalloc(sizeof(ArVisualFilter))" not in source, "ArVisualFilter must use normal C++ object lifetime")

    shader = read("data/effects/arvisual.effect")
    require("if (abs(arvisual_shader_abi_v3 - 3.0) > 0.001)" in shader, "Shader ABI value guard is missing")

    installer = read("packaging/windows/arvisual.iss")
""",
    1,
)
write("scripts/validate-repository.py", validator)

replace_once(
    ".github/workflows/quality.yml",
    '              "docs/assets/product-preview.svg",\n',
    '              "docs/assets/product-preview.svg",\n              "docs/assets/social-preview.svg",\n              "docs/assets/social-preview.png",\n',
)

changelog = read("CHANGELOG.md")
if f"## v{VERSION}" not in changelog:
    entry = f"""# Changelog

## v{VERSION} — 2026-07-18

- Replaced C-style allocation of the native filter state with normal C++ object lifetime and allocation-failure handling.
- Added safe fallback behavior when Smart Auto GPU analysis resources are only partially available; the normal color grade remains active.
- Added shader compiler diagnostics to the OBS log and a runtime ABI value guard that returns the unmodified source on mismatch.
- Pinned the external OBS plugin template to an immutable reviewed commit for repeatable local and release builds.
- Added a 1200×630 PNG social preview for reliable Open Graph and Twitter sharing.
- Added CI contracts for runtime hardening, pinned dependencies, social metadata, version consistency, and color safety.
- Code signing remains intentionally optional and is not required for this release.

"""
    changelog = changelog.replace("# Changelog\n\n", entry, 1)
    write("CHANGELOG.md", changelog)

print(f"Applied ArVisual v{VERSION} hardening migration")
