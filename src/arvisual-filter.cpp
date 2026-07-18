#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/vec4.h>
#include <util/platform.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <new>

#define SETTING_PRESET "preset"
#define SETTING_PRESET_APPLIED "preset_applied" /* hidden bookkeeping, not a UI property */
#define SETTING_SMART "smart_auto"
#define SETTING_MASTER "master"
#define SETTING_ENHANCE "enhance"
#define SETTING_COLOR_POP "color_pop"
#define SETTING_CLEAN_WHITE "clean_white"
#define SETTING_CLARITY "clarity"
#define SETTING_SKIN_PROTECT "skin_protect"
#define SETTING_SKIN_BEAUTY "skin_beauty"
#define SETTING_HEALTHY_TONE "healthy_tone"
#define SETTING_TOY_GLOSS "toy_gloss"
#define SETTING_DEPTH_POP "depth_pop"
#define SETTING_HIGHLIGHT_GUARD "highlight_guard"
#define SETTING_PERFORMANCE "performance"
#define SETTING_BYPASS "bypass"

#define PRESET_CUSTOM_ID "custom"

#define TEXT_FILTER_NAME obs_module_text("ArVisual.FilterName")
#define TEXT_PRESET obs_module_text("ArVisual.Preset")
#define TEXT_SMART obs_module_text("ArVisual.SmartAuto")
#define TEXT_MASTER obs_module_text("ArVisual.Master")
#define TEXT_ENHANCE obs_module_text("ArVisual.Enhance")
#define TEXT_COLOR_POP obs_module_text("ArVisual.ColorPop")
#define TEXT_CLEAN_WHITE obs_module_text("ArVisual.CleanWhite")
#define TEXT_CLARITY obs_module_text("ArVisual.VisualClarity")
#define TEXT_SKIN_PROTECT obs_module_text("ArVisual.SkinProtect")
#define TEXT_SKIN_BEAUTY obs_module_text("ArVisual.SkinBeauty")
#define TEXT_HEALTHY_TONE obs_module_text("ArVisual.HealthyTone")
#define TEXT_TOY_GLOSS obs_module_text("ArVisual.ToyGloss")
#define TEXT_DEPTH_POP obs_module_text("ArVisual.DepthPop")
#define TEXT_HIGHLIGHT_GUARD obs_module_text("ArVisual.HighlightGuard")
#define TEXT_PERFORMANCE obs_module_text("ArVisual.Performance")
#define TEXT_BYPASS obs_module_text("ArVisual.Bypass")

namespace {

/* Scene analysis buffer size. 64x36 keeps the 16:9 shape, is a trivial GPU
 * blit, and gives 2304 samples for stable statistics on the CPU. */
constexpr uint32_t ANALYSIS_W = 64;
constexpr uint32_t ANALYSIS_H = 36;
constexpr float SHADER_ABI = 3.0f;

struct PresetValues {
    const char *id;
    const char *text;
    int enhance;
    int color_pop;
    int clean_white;
    int clarity;
    int skin_beauty;
    int healthy_tone;
    int toy_gloss;
    int depth_pop;
    int highlight_guard;
    const char *skin_protect;
    const char *performance;
};

constexpr PresetValues PRESETS[] = {
    {"masari_dopamine", "Preset.MasAriDopamine", 78, 86, 72, 68, 72, 62, 48, 76, 94, "strong", "ultra"},
    {"kids_toy_pop", "Preset.KidsToyPop", 84, 94, 78, 72, 70, 58, 66, 82, 94, "strong", "ultra"},
    {"cars_candy_color", "Preset.CarsCandyColor", 88, 96, 64, 78, 55, 45, 72, 88, 94, "normal", "ultra"},
    {"product_clean_pop", "Preset.ProductCleanPop", 76, 82, 78, 76, 52, 45, 54, 80, 94, "normal", "ultra"},
    {"beauty_bright", "Preset.BeautyBright", 62, 52, 70, 24, 88, 82, 18, 42, 96, "strong", "standard"},
    {"gaming_vivid", "Preset.GamingVivid", 82, 96, 50, 78, 40, 35, 62, 88, 92, "normal", "ultra"},
    {"natural_safe", "Preset.NaturalSafe", 36, 28, 42, 18, 42, 38, 10, 24, 96, "strong", "lite"},
    {"cinematic_candy", "Preset.CinematicCandy", 68, 68, 34, 54, 50, 42, 36, 66, 94, "normal", "standard"},
};

float pct(obs_data_t *settings, const char *key)
{
    return std::clamp(static_cast<float>(obs_data_get_int(settings, key)) / 100.0f, 0.0f, 1.0f);
}

float skin_mode_to_float(const char *mode)
{
    if (!mode || !*mode)
        return 0.65f;
    if (std::strcmp(mode, "off") == 0)
        return 0.0f;
    if (std::strcmp(mode, "strong") == 0)
        return 1.0f;
    return 0.65f;
}

const PresetValues *find_preset(const char *id)
{
    if (!id || !*id)
        return nullptr;

    for (const auto &preset : PRESETS) {
        if (std::strcmp(preset.id, id) == 0)
            return &preset;
    }

    return nullptr;
}

void apply_preset_to_settings(obs_data_t *settings, const PresetValues &preset)
{
    obs_data_set_int(settings, SETTING_ENHANCE, preset.enhance);
    obs_data_set_int(settings, SETTING_COLOR_POP, preset.color_pop);
    obs_data_set_int(settings, SETTING_CLEAN_WHITE, preset.clean_white);
    obs_data_set_int(settings, SETTING_CLARITY, preset.clarity);
    obs_data_set_int(settings, SETTING_SKIN_BEAUTY, preset.skin_beauty);
    obs_data_set_int(settings, SETTING_HEALTHY_TONE, preset.healthy_tone);
    obs_data_set_int(settings, SETTING_TOY_GLOSS, preset.toy_gloss);
    obs_data_set_int(settings, SETTING_DEPTH_POP, preset.depth_pop);
    obs_data_set_int(settings, SETTING_HIGHLIGHT_GUARD, preset.highlight_guard);
    obs_data_set_string(settings, SETTING_SKIN_PROTECT, preset.skin_protect);
    obs_data_set_string(settings, SETTING_PERFORMANCE, preset.performance);
    /* Each preset starts at its designed dose; the master knob then scales it. */
    obs_data_set_int(settings, SETTING_MASTER, 100);
}

bool settings_match_preset(obs_data_t *settings, const PresetValues &preset)
{
    if (obs_data_get_int(settings, SETTING_ENHANCE) != preset.enhance)
        return false;
    if (obs_data_get_int(settings, SETTING_COLOR_POP) != preset.color_pop)
        return false;
    if (obs_data_get_int(settings, SETTING_CLEAN_WHITE) != preset.clean_white)
        return false;
    if (obs_data_get_int(settings, SETTING_CLARITY) != preset.clarity)
        return false;
    if (obs_data_get_int(settings, SETTING_SKIN_BEAUTY) != preset.skin_beauty)
        return false;
    if (obs_data_get_int(settings, SETTING_HEALTHY_TONE) != preset.healthy_tone)
        return false;
    if (obs_data_get_int(settings, SETTING_TOY_GLOSS) != preset.toy_gloss)
        return false;
    if (obs_data_get_int(settings, SETTING_DEPTH_POP) != preset.depth_pop)
        return false;
    if (obs_data_get_int(settings, SETTING_HIGHLIGHT_GUARD) != preset.highlight_guard)
        return false;
    if (std::strcmp(obs_data_get_string(settings, SETTING_SKIN_PROTECT), preset.skin_protect) != 0)
        return false;
    if (std::strcmp(obs_data_get_string(settings, SETTING_PERFORMANCE), preset.performance) != 0)
        return false;
    return true;
}

/* Preset combo callback.
 *
 * OBS invokes every modified callback when the properties dialog opens
 * (obs_properties_apply_settings), not only when the user changes the combo.
 * Without a guard this stomps hand-tuned slider values on every dialog open.
 * We therefore only apply preset values when the selected preset id actually
 * differs from the last id we applied (tracked in a hidden setting). */
bool preset_modified(obs_properties_t *, obs_property_t *, obs_data_t *settings)
{
    const char *id = obs_data_get_string(settings, SETTING_PRESET);

    if (id && std::strcmp(id, PRESET_CUSTOM_ID) == 0)
        return false;

    const char *applied = obs_data_get_string(settings, SETTING_PRESET_APPLIED);
    if (applied && id && std::strcmp(applied, id) == 0)
        return false;

    const PresetValues *preset = find_preset(id);
    if (!preset)
        return false;

    apply_preset_to_settings(settings, *preset);
    obs_data_set_string(settings, SETTING_PRESET_APPLIED, preset->id);
    return true;
}

/* Slider/list callback: if the values no longer match the selected preset,
 * mark the state as Custom so the user can see (and trust) that their manual
 * tuning is what is active. Returns false to avoid rebuilding the dialog
 * while a slider is being dragged. */
bool tuning_modified(obs_properties_t *, obs_property_t *, obs_data_t *settings)
{
    const char *id = obs_data_get_string(settings, SETTING_PRESET);
    if (!id || std::strcmp(id, PRESET_CUSTOM_ID) == 0)
        return false;

    const PresetValues *preset = find_preset(id);
    if (!preset)
        return false;

    if (!settings_match_preset(settings, *preset)) {
        obs_data_set_string(settings, SETTING_PRESET, PRESET_CUSTOM_ID);
        obs_data_set_string(settings, SETTING_PRESET_APPLIED, PRESET_CUSTOM_ID);
    }

    return false;
}

/* Scene statistics measured on the CPU from a 64x36 readback, smoothed with
 * an exponential moving average so the grade never pumps or flickers. */
struct SceneStats {
    float p10_luma = 0.10f;
    float median_luma = 0.40f;
    float p90_luma = 0.78f;
    float p98_luma = 0.92f;
    float mean_saturation = 0.30f;
    float p90_saturation = 0.72f;
    float shadow_frac = 0.0f;
    float near_clip_frac = 0.0f;
    float vivid_frac = 0.0f;
    float hot_vivid_frac = 0.0f;
    float neutral_frac = 0.30f;
    float colored_frac = 0.10f;
    bool primed = false;
};

struct ArVisualFilter {
    obs_source_t *context = nullptr;
    gs_effect_t *effect = nullptr;

    gs_eparam_t *image_param = nullptr;
    gs_eparam_t *shader_abi_param = nullptr;
    gs_eparam_t *master_param = nullptr;
    gs_eparam_t *enhance_param = nullptr;
    gs_eparam_t *color_pop_param = nullptr;
    gs_eparam_t *clean_white_param = nullptr;
    gs_eparam_t *clarity_param = nullptr;
    gs_eparam_t *skin_protect_param = nullptr;
    gs_eparam_t *skin_beauty_param = nullptr;
    gs_eparam_t *healthy_tone_param = nullptr;
    gs_eparam_t *toy_gloss_param = nullptr;
    gs_eparam_t *depth_pop_param = nullptr;
    gs_eparam_t *highlight_guard_param = nullptr;
    gs_eparam_t *performance_param = nullptr;
    gs_eparam_t *texel_size_param = nullptr;
    gs_eparam_t *smart_exposure_param = nullptr;
    gs_eparam_t *smart_pop_param = nullptr;
    gs_eparam_t *smart_highlight_param = nullptr;
    gs_eparam_t *smart_shadow_param = nullptr;
    gs_eparam_t *smart_strength_param = nullptr;
    gs_eparam_t *smart_chroma_limit_param = nullptr;
    gs_eparam_t *smart_clean_param = nullptr;
    gs_eparam_t *smart_separation_param = nullptr;
    bool effect_compatible = false;
    bool smart_resources_ready = false;

    /* Smart engine graphics resources. */
    gs_texrender_t *input_tr = nullptr;
    gs_texrender_t *analysis_tr = nullptr;
    gs_stagesurf_t *stage[2] = {nullptr, nullptr};
    bool stage_valid[2] = {false, false};
    int stage_write = 0;
    uint64_t last_ns = 0;

    SceneStats stats;

    /* Adaptive outputs handed to the shader. Neutral values are used when
     * Smart Auto is off or before the first readback arrives. */
    float smart_exposure = 0.0f;
    float smart_pop = 1.0f;
    float smart_highlight = 0.0f;
    float smart_shadow = 0.0f;
    float smart_strength = 1.0f;
    float smart_chroma_limit = 0.985f;
    float smart_clean = 0.0f;
    float smart_separation = 0.0f;

    bool smart = true;
    float master = 1.0f;
    float enhance = 0.78f;
    float color_pop = 0.86f;
    float clean_white = 0.72f;
    float clarity = 0.68f;
    float skin_protect = 1.0f;
    float skin_beauty = 0.72f;
    float healthy_tone = 0.62f;
    float toy_gloss = 0.48f;
    float depth_pop = 0.76f;
    float highlight_guard = 0.94f;
    float performance = 1.0f;
    bool bypass = false;
};

const char *filter_name(void *)
{
    return TEXT_FILTER_NAME;
}

void update(void *data, obs_data_t *settings)
{
    auto *filter = static_cast<ArVisualFilter *>(data);

    filter->smart = obs_data_get_bool(settings, SETTING_SMART);

    /* Master is 0..200% in the UI, 0.0..2.0 in the shader. */
    filter->master = std::clamp(static_cast<float>(obs_data_get_int(settings, SETTING_MASTER)) / 100.0f, 0.0f, 2.0f);

    filter->enhance = pct(settings, SETTING_ENHANCE);
    filter->color_pop = pct(settings, SETTING_COLOR_POP);
    filter->clean_white = pct(settings, SETTING_CLEAN_WHITE);
    filter->clarity = pct(settings, SETTING_CLARITY);
    filter->skin_beauty = pct(settings, SETTING_SKIN_BEAUTY);
    filter->healthy_tone = pct(settings, SETTING_HEALTHY_TONE);
    filter->toy_gloss = pct(settings, SETTING_TOY_GLOSS);
    filter->depth_pop = pct(settings, SETTING_DEPTH_POP);
    filter->highlight_guard = pct(settings, SETTING_HIGHLIGHT_GUARD);
    filter->skin_protect = skin_mode_to_float(obs_data_get_string(settings, SETTING_SKIN_PROTECT));
    filter->bypass = obs_data_get_bool(settings, SETTING_BYPASS);

    const char *performance = obs_data_get_string(settings, SETTING_PERFORMANCE);
    if (performance && std::strcmp(performance, "lite") == 0)
        filter->performance = 0.0f;
    else if (performance && std::strcmp(performance, "ultra") == 0)
        filter->performance = 1.0f;
    else
        filter->performance = 0.5f;
}

void destroy(void *data)
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

void *create(obs_data_t *settings, obs_source_t *context)
{
    auto *filter = new (std::nothrow) ArVisualFilter{};
    if (!filter) {
        blog(LOG_ERROR, "[ArVisual] Failed to allocate filter state");
        return nullptr;
    }

    filter->context = context;

    char *effect_path = obs_module_file("effects/arvisual.effect");
    if (!effect_path) {
        blog(LOG_ERROR, "[ArVisual] effects/arvisual.effect not found in module data path");
        delete filter;
        return nullptr;
    }

    char *effect_errors = nullptr;

    obs_enter_graphics();
    filter->effect = gs_effect_create_from_file(effect_path, &effect_errors);
    if (filter->effect) {
        filter->image_param = gs_effect_get_param_by_name(filter->effect, "image");
        filter->shader_abi_param = gs_effect_get_param_by_name(filter->effect, "arvisual_shader_abi_v3");
        filter->master_param = gs_effect_get_param_by_name(filter->effect, SETTING_MASTER);
        filter->enhance_param = gs_effect_get_param_by_name(filter->effect, SETTING_ENHANCE);
        filter->color_pop_param = gs_effect_get_param_by_name(filter->effect, SETTING_COLOR_POP);
        filter->clean_white_param = gs_effect_get_param_by_name(filter->effect, SETTING_CLEAN_WHITE);
        filter->clarity_param = gs_effect_get_param_by_name(filter->effect, SETTING_CLARITY);
        filter->skin_protect_param = gs_effect_get_param_by_name(filter->effect, SETTING_SKIN_PROTECT);
        filter->skin_beauty_param = gs_effect_get_param_by_name(filter->effect, SETTING_SKIN_BEAUTY);
        filter->healthy_tone_param = gs_effect_get_param_by_name(filter->effect, SETTING_HEALTHY_TONE);
        filter->toy_gloss_param = gs_effect_get_param_by_name(filter->effect, SETTING_TOY_GLOSS);
        filter->depth_pop_param = gs_effect_get_param_by_name(filter->effect, SETTING_DEPTH_POP);
        filter->highlight_guard_param = gs_effect_get_param_by_name(filter->effect, SETTING_HIGHLIGHT_GUARD);
        filter->performance_param = gs_effect_get_param_by_name(filter->effect, SETTING_PERFORMANCE);
        filter->texel_size_param = gs_effect_get_param_by_name(filter->effect, "texel_size");
        filter->smart_exposure_param = gs_effect_get_param_by_name(filter->effect, "smart_exposure");
        filter->smart_pop_param = gs_effect_get_param_by_name(filter->effect, "smart_pop");
        filter->smart_highlight_param = gs_effect_get_param_by_name(filter->effect, "smart_highlight");
        filter->smart_shadow_param = gs_effect_get_param_by_name(filter->effect, "smart_shadow");
        filter->smart_strength_param = gs_effect_get_param_by_name(filter->effect, "smart_strength");
        filter->smart_chroma_limit_param = gs_effect_get_param_by_name(filter->effect, "smart_chroma_limit");
        filter->smart_clean_param = gs_effect_get_param_by_name(filter->effect, "smart_clean");
        filter->smart_separation_param = gs_effect_get_param_by_name(filter->effect, "smart_separation");

        filter->effect_compatible =
            filter->image_param && filter->shader_abi_param && filter->master_param && filter->enhance_param &&
            filter->color_pop_param && filter->clean_white_param && filter->clarity_param &&
            filter->skin_protect_param && filter->skin_beauty_param && filter->healthy_tone_param &&
            filter->toy_gloss_param && filter->depth_pop_param && filter->highlight_guard_param &&
            filter->performance_param && filter->texel_size_param && filter->smart_exposure_param &&
            filter->smart_pop_param && filter->smart_highlight_param && filter->smart_shadow_param &&
            filter->smart_strength_param && filter->smart_chroma_limit_param && filter->smart_clean_param &&
            filter->smart_separation_param;

        if (filter->effect_compatible) {
            filter->input_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->analysis_tr = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
            filter->stage[0] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
            filter->stage[1] = gs_stagesurface_create(ANALYSIS_W, ANALYSIS_H, GS_RGBA);
            filter->smart_resources_ready = filter->input_tr && filter->analysis_tr && filter->stage[0] && filter->stage[1];
        }
    }
    obs_leave_graphics();

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

    if (!filter->effect) {
        blog(LOG_ERROR, "[ArVisual] Failed to load effects/arvisual.effect");
        destroy(filter);
        return nullptr;
    }

    if (!filter->effect_compatible) {
        blog(LOG_ERROR,
             "[ArVisual] Shader interface mismatch. Reinstall the plugin so arvisual.dll and "
             "effects/arvisual.effect come from the same package; the filter will be bypassed.");
    }

    update(filter, settings);
    return filter;
}

/* Read the analysis surface staged on a previous frame and fold the fresh
 * statistics into the EMA state. Returns quietly if nothing is ready yet. */
void read_scene_stats(ArVisualFilter *filter, float dt)
{
    if (!filter->smart_resources_ready)
        return;

    const int read_idx = 1 - filter->stage_write;
    if (!filter->stage_valid[read_idx] || !filter->stage[read_idx])
        return;

    uint8_t *ptr = nullptr;
    uint32_t linesize = 0;
    if (!gs_stagesurface_map(filter->stage[read_idx], &ptr, &linesize))
        return;

    std::array<uint32_t, 256> histogram{};
    std::array<uint32_t, 256> saturation_histogram{};
    double sum_saturation = 0.0;
    uint32_t active = 0;
    uint32_t saturation_samples = 0;
    uint32_t shadows = 0;
    uint32_t near_clip = 0;
    uint32_t vivid = 0;
    uint32_t hot_vivid = 0;
    uint32_t neutral = 0;
    uint32_t colored = 0;

    for (uint32_t row = 0; row < ANALYSIS_H; row++) {
        const uint8_t *px = ptr + static_cast<size_t>(row) * linesize;
        for (uint32_t col = 0; col < ANALYSIS_W; col++, px += 4) {
            const float r = px[0] * (1.0f / 255.0f);
            const float g = px[1] * (1.0f / 255.0f);
            const float b = px[2] * (1.0f / 255.0f);
            const float a = px[3] * (1.0f / 255.0f);
            const float y = 0.2126f * r + 0.7152f * g + 0.0722f * b;
            const float max_c = std::max({r, g, b});

            /* Ignore transparent pixels and near-black letterbox/background
             * pixels. They contain no exposure information and previously
             * made low-coverage scenes look falsely underexposed. */
            if (a < 0.035f || (y < 0.020f && max_c < 0.035f))
                continue;

            const float min_c = std::min({r, g, b});
            const float saturation = max_c > 0.001f ? (max_c - min_c) / max_c : 0.0f;
            const uint32_t bin = std::min(static_cast<uint32_t>(y * 255.0f + 0.5f), 255u);
            histogram[bin]++;
            active++;

            if (y >= 0.070f) {
                sum_saturation += saturation;
                const uint32_t saturation_bin =
                    std::min(static_cast<uint32_t>(saturation * 255.0f + 0.5f), 255u);
                saturation_histogram[saturation_bin]++;
                saturation_samples++;
            }
            if (y < 0.095f)
                shadows++;
            if (max_c >= 0.975f)
                near_clip++;
            if (saturation >= 0.78f)
                vivid++;
            if (saturation >= 0.70f && max_c >= 0.92f)
                hot_vivid++;
            if (y >= 0.15f && saturation < 0.12f)
                neutral++;
            if (y >= 0.08f && saturation > 0.35f)
                colored++;
        }
    }

    gs_stagesurface_unmap(filter->stage[read_idx]);

    if (active < 8)
        return;

    const auto percentile = [](const std::array<uint32_t, 256> &bins, uint32_t total, float q) {
        const uint32_t wanted = std::max(1u, static_cast<uint32_t>(std::ceil(q * static_cast<float>(total))));
        uint32_t cumulative = 0;
        for (uint32_t i = 0; i < bins.size(); i++) {
            cumulative += bins[i];
            if (cumulative >= wanted)
                return static_cast<float>(i) * (1.0f / 255.0f);
        }
        return 1.0f;
    };

    const float p10 = percentile(histogram, active, 0.10f);
    const float p50 = percentile(histogram, active, 0.50f);
    const float p90 = percentile(histogram, active, 0.90f);
    const float p98 = percentile(histogram, active, 0.98f);
    const float mean_sat = saturation_samples ? static_cast<float>(sum_saturation) / static_cast<float>(saturation_samples) : 0.0f;
    const float p90_sat = saturation_samples ? percentile(saturation_histogram, saturation_samples, 0.90f) : 0.0f;
    const float inv_active = 1.0f / static_cast<float>(active);
    const float frac_shadow = static_cast<float>(shadows) * inv_active;
    const float frac_near_clip = static_cast<float>(near_clip) * inv_active;
    const float frac_vivid = static_cast<float>(vivid) * inv_active;
    const float frac_hot_vivid = static_cast<float>(hot_vivid) * inv_active;
    const float frac_neutral = static_cast<float>(neutral) * inv_active;
    const float frac_colored = static_cast<float>(colored) * inv_active;

    SceneStats &s = filter->stats;
    if (!s.primed) {
        s.p10_luma = p10;
        s.median_luma = p50;
        s.p90_luma = p90;
        s.p98_luma = p98;
        s.mean_saturation = mean_sat;
        s.p90_saturation = p90_sat;
        s.shadow_frac = frac_shadow;
        s.near_clip_frac = frac_near_clip;
        s.vivid_frac = frac_vivid;
        s.hot_vivid_frac = frac_hot_vivid;
        s.neutral_frac = frac_neutral;
        s.colored_frac = frac_colored;
        s.primed = true;
    } else {
        /* tau = 0.65 s: fast enough to follow scene cuts, slow enough that the
         * grade never visibly pumps with in-scene motion. */
        const float alpha = 1.0f - std::exp(-dt / 0.65f);
        s.p10_luma += (p10 - s.p10_luma) * alpha;
        s.median_luma += (p50 - s.median_luma) * alpha;
        s.p90_luma += (p90 - s.p90_luma) * alpha;
        s.p98_luma += (p98 - s.p98_luma) * alpha;
        s.mean_saturation += (mean_sat - s.mean_saturation) * alpha;
        s.p90_saturation += (p90_sat - s.p90_saturation) * alpha;
        s.shadow_frac += (frac_shadow - s.shadow_frac) * alpha;
        s.near_clip_frac += (frac_near_clip - s.near_clip_frac) * alpha;
        s.vivid_frac += (frac_vivid - s.vivid_frac) * alpha;
        s.hot_vivid_frac += (frac_hot_vivid - s.hot_vivid_frac) * alpha;
        s.neutral_frac += (frac_neutral - s.neutral_frac) * alpha;
        s.colored_frac += (frac_colored - s.colored_frac) * alpha;
    }

    /* Percentile-based decisions are stable across letterboxing and sparse
     * subjects. Smart Auto is asymmetric by design: it may strongly reduce a
     * dangerous creative dose, but only very gently increases a muted scene. */
    /* Upper-key exposure preserves intentionally low-key scenes. A healthy
     * highlight distribution matters more than how much black background the
     * composition contains. */
    const float upper_key = s.p90_luma * 0.72f + s.p98_luma * 0.28f;
    filter->smart_exposure = std::clamp((0.64f - upper_key) * 0.09f, -0.025f, 0.012f);

    const float p90_pressure = std::clamp((s.p90_luma - 0.80f) / 0.16f, 0.0f, 1.0f);
    const float p98_pressure = std::clamp((s.p98_luma - 0.91f) / 0.075f, 0.0f, 1.0f);
    const float clip_pressure = std::clamp((s.near_clip_frac - 0.012f) * 10.0f, 0.0f, 1.0f);
    const float hot_pressure = std::clamp((s.hot_vivid_frac - 0.008f) * 12.0f, 0.0f, 1.0f);
    filter->smart_highlight = std::max({p90_pressure * 0.70f, p98_pressure, clip_pressure, hot_pressure});

    const float sat_pressure = std::clamp((s.mean_saturation - 0.32f) / 0.34f, 0.0f, 1.0f);
    const float tail_sat_pressure = std::clamp((s.p90_saturation - 0.68f) / 0.28f, 0.0f, 1.0f);
    const float vivid_pressure = std::clamp((s.vivid_frac - 0.16f) / 0.40f, 0.0f, 1.0f);
    const float color_risk = std::max({sat_pressure, tail_sat_pressure, vivid_pressure, hot_pressure});
    const float muted_lift = std::clamp((0.18f - s.mean_saturation) * 0.16f, 0.0f, 0.025f);

    filter->smart_pop = std::clamp(1.0f + muted_lift - color_risk * 0.48f, 0.52f, 1.025f);
    filter->smart_strength = std::clamp(1.0f - color_risk * 0.34f - filter->smart_highlight * 0.16f, 0.58f, 1.0f);
    filter->smart_chroma_limit = std::clamp(0.985f - color_risk * 0.060f - filter->smart_highlight * 0.025f, 0.90f, 0.985f);

    /* Neutral-dominant scenes with a smaller colored subject are typically
     * product/demo/tutorial compositions. Clean the neutral field harder and
     * create hero color separation without increasing blanket saturation. */
    const float neutral_dominance = std::clamp((s.neutral_frac - 0.30f) / 0.45f, 0.0f, 1.0f);
    const float colored_presence = std::clamp((s.colored_frac - 0.04f) / 0.22f, 0.0f, 1.0f);
    const float colorful_scene = std::clamp((s.mean_saturation - 0.24f) / 0.25f, 0.0f, 1.0f);
    filter->smart_clean = neutral_dominance * (1.0f - colorful_scene);
    filter->smart_separation = neutral_dominance * colored_presence * (1.0f - filter->smart_highlight * 0.45f);

    const float low_percentile_pressure = std::clamp((0.085f - s.p10_luma) / 0.075f, 0.0f, 1.0f);
    const float shadow_area_pressure = std::clamp((s.shadow_frac - 0.24f) * 2.2f, 0.0f, 1.0f);
    filter->smart_shadow = std::max(low_percentile_pressure, shadow_area_pressure);
}

void set_shader_params(ArVisualFilter *filter, uint32_t width, uint32_t height)
{
    const bool smart = filter->smart;

    gs_effect_set_float(filter->shader_abi_param, SHADER_ABI);
    if (filter->master_param) gs_effect_set_float(filter->master_param, filter->master);
    if (filter->enhance_param) gs_effect_set_float(filter->enhance_param, filter->enhance);
    if (filter->color_pop_param) gs_effect_set_float(filter->color_pop_param, filter->color_pop);
    if (filter->clean_white_param) gs_effect_set_float(filter->clean_white_param, filter->clean_white);
    if (filter->clarity_param) gs_effect_set_float(filter->clarity_param, filter->clarity);
    if (filter->skin_protect_param) gs_effect_set_float(filter->skin_protect_param, filter->skin_protect);
    if (filter->skin_beauty_param) gs_effect_set_float(filter->skin_beauty_param, filter->skin_beauty);
    if (filter->healthy_tone_param) gs_effect_set_float(filter->healthy_tone_param, filter->healthy_tone);
    if (filter->toy_gloss_param) gs_effect_set_float(filter->toy_gloss_param, filter->toy_gloss);
    if (filter->depth_pop_param) gs_effect_set_float(filter->depth_pop_param, filter->depth_pop);
    if (filter->highlight_guard_param) gs_effect_set_float(filter->highlight_guard_param, filter->highlight_guard);
    if (filter->performance_param) gs_effect_set_float(filter->performance_param, filter->performance);

    if (filter->smart_exposure_param)
        gs_effect_set_float(filter->smart_exposure_param, smart ? filter->smart_exposure : 0.0f);
    if (filter->smart_pop_param)
        gs_effect_set_float(filter->smart_pop_param, smart ? filter->smart_pop : 1.0f);
    if (filter->smart_highlight_param)
        gs_effect_set_float(filter->smart_highlight_param, smart ? filter->smart_highlight : 0.0f);
    if (filter->smart_shadow_param)
        gs_effect_set_float(filter->smart_shadow_param, smart ? filter->smart_shadow : 0.0f);
    if (filter->smart_strength_param)
        gs_effect_set_float(filter->smart_strength_param, smart ? filter->smart_strength : 1.0f);
    if (filter->smart_chroma_limit_param)
        gs_effect_set_float(filter->smart_chroma_limit_param, smart ? filter->smart_chroma_limit : 0.985f);
    if (filter->smart_clean_param)
        gs_effect_set_float(filter->smart_clean_param, smart ? filter->smart_clean : 0.0f);
    if (filter->smart_separation_param)
        gs_effect_set_float(filter->smart_separation_param, smart ? filter->smart_separation : 0.0f);

    if (filter->texel_size_param) {
        vec2 texel_size;
        vec2_set(&texel_size, width > 0 ? 1.0f / static_cast<float>(width) : 0.0f,
                 height > 0 ? 1.0f / static_cast<float>(height) : 0.0f);
        gs_effect_set_vec2(filter->texel_size_param, &texel_size);
    }
}

void render(void *data, gs_effect_t *effect)
{
    UNUSED_PARAMETER(effect);

    auto *filter = static_cast<ArVisualFilter *>(data);

    if (!filter->effect_compatible || filter->bypass || filter->master <= 0.001f) {
        obs_source_skip_video_filter(filter->context);
        return;
    }

    obs_source_t *target = obs_filter_get_target(filter->context);
    const uint32_t width = target ? obs_source_get_width(target) : 0;
    const uint32_t height = target ? obs_source_get_height(target) : 0;

    if (!width || !height) {
        obs_source_skip_video_filter(filter->context);
        return;
    }

    const uint64_t now = os_gettime_ns();
    float dt = 0.016f;
    if (filter->last_ns)
        dt = std::clamp(static_cast<float>(now - filter->last_ns) * 1e-9f, 0.001f, 0.25f);
    filter->last_ns = now;

    if (filter->smart && filter->smart_resources_ready)
        read_scene_stats(filter, dt);

    /* Pin the whole ArVisual pipeline to NON-LINEAR (gamma / sRGB-encoded)
     * values. Without this, obs_source_process_filter_* may bind the input
     * as an sRGB view depending on global state, feeding the shader linear
     * values - the engine is designed and tuned in gamma space, so the grade
     * would silently change strength and character between OBS versions. */
    const bool prev_linear_srgb = gs_set_linear_srgb(false);

    if (!obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING)) {
        gs_set_linear_srgb(prev_linear_srgb);
        return;
    }

    /* Stage 1: capture the filter input into our own texture so the same
     * frame can feed both the analysis blit and the final grade. */
    bool captured = false;
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

    if (!captured) {
        /* Texrender unavailable: fall back to the classic single-pass path so
         * the stream never loses the grade. process_filter_end is still owed. */
        set_shader_params(filter, width, height);
        const bool prev_fb = gs_framebuffer_srgb_enabled();
        gs_enable_framebuffer_srgb(false);
        gs_blend_state_push();
        gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
        obs_source_process_filter_end(filter->context, filter->effect, 0, 0);
        gs_blend_state_pop();
        gs_enable_framebuffer_srgb(prev_fb);
        gs_set_linear_srgb(prev_linear_srgb);
        return;
    }

    gs_texture_t *input_tex = gs_texrender_get_texture(filter->input_tr);

    /* Stage 2: 64x36 analysis blit + async stage. Read happens next frame. */
    if (filter->smart && filter->smart_resources_ready && input_tex) {
        gs_texrender_reset(filter->analysis_tr);
        if (gs_texrender_begin(filter->analysis_tr, ANALYSIS_W, ANALYSIS_H)) {
            vec4 clear_color;
            vec4_zero(&clear_color);
            gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
            gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);

            gs_effect_t *pass = obs_get_base_effect(OBS_EFFECT_DEFAULT);
            gs_eparam_t *pass_image = gs_effect_get_param_by_name(pass, "image");
            gs_effect_set_texture(pass_image, input_tex);
            while (gs_effect_loop(pass, "Draw"))
                gs_draw_sprite(input_tex, 0, width, height);

            gs_texrender_end(filter->analysis_tr);

            gs_texture_t *analysis_tex = gs_texrender_get_texture(filter->analysis_tr);
            if (analysis_tex && filter->stage[filter->stage_write]) {
                gs_stage_texture(filter->stage[filter->stage_write], analysis_tex);
                filter->stage_valid[filter->stage_write] = true;
                filter->stage_write = 1 - filter->stage_write;
            }
        }
    }

    /* Stage 3: final grade from the captured input to the output target. */
    set_shader_params(filter, width, height);
    if (filter->image_param)
        gs_effect_set_texture(filter->image_param, input_tex);

    const bool prev_fb = gs_framebuffer_srgb_enabled();
    gs_enable_framebuffer_srgb(false);
    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
    while (gs_effect_loop(filter->effect, "Draw"))
        gs_draw_sprite(input_tex, 0, width, height);
    gs_blend_state_pop();
    gs_enable_framebuffer_srgb(prev_fb);
    gs_set_linear_srgb(prev_linear_srgb);
}

obs_properties_t *properties(void *)
{
    obs_properties_t *props = obs_properties_create();

    obs_property_t *preset = obs_properties_add_list(props, SETTING_PRESET, TEXT_PRESET,
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (const auto &item : PRESETS)
        obs_property_list_add_string(preset, obs_module_text(item.text), item.id);
    obs_property_list_add_string(preset, obs_module_text("Preset.Custom"), PRESET_CUSTOM_ID);
    obs_property_set_modified_callback(preset, preset_modified);

    obs_properties_add_bool(props, SETTING_SMART, TEXT_SMART);

    obs_property_t *master = obs_properties_add_int_slider(props, SETTING_MASTER, TEXT_MASTER, 0, 200, 1);
    obs_property_int_set_suffix(master, " %");

    const char *tuning_keys[] = {
        SETTING_ENHANCE,     SETTING_COLOR_POP,    SETTING_CLEAN_WHITE, SETTING_CLARITY,
        SETTING_SKIN_BEAUTY, SETTING_HEALTHY_TONE, SETTING_DEPTH_POP,   SETTING_TOY_GLOSS,
        SETTING_HIGHLIGHT_GUARD,
    };
    const char *tuning_texts[] = {
        TEXT_ENHANCE,     TEXT_COLOR_POP,    TEXT_CLEAN_WHITE, TEXT_CLARITY,
        TEXT_SKIN_BEAUTY, TEXT_HEALTHY_TONE, TEXT_DEPTH_POP,   TEXT_TOY_GLOSS,
        TEXT_HIGHLIGHT_GUARD,
    };
    for (size_t i = 0; i < sizeof(tuning_keys) / sizeof(tuning_keys[0]); i++) {
        obs_property_t *slider = obs_properties_add_int_slider(props, tuning_keys[i], tuning_texts[i], 0, 100, 1);
        obs_property_set_modified_callback(slider, tuning_modified);
    }

    obs_property_t *skin = obs_properties_add_list(props, SETTING_SKIN_PROTECT, TEXT_SKIN_PROTECT,
                                                   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(skin, obs_module_text("Skin.Off"), "off");
    obs_property_list_add_string(skin, obs_module_text("Skin.Normal"), "normal");
    obs_property_list_add_string(skin, obs_module_text("Skin.Strong"), "strong");
    obs_property_set_modified_callback(skin, tuning_modified);

    obs_property_t *performance = obs_properties_add_list(props, SETTING_PERFORMANCE, TEXT_PERFORMANCE,
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(performance, obs_module_text("Performance.Lite"), "lite");
    obs_property_list_add_string(performance, obs_module_text("Performance.Standard"), "standard");
    obs_property_list_add_string(performance, obs_module_text("Performance.Ultra"), "ultra");
    obs_property_set_modified_callback(performance, tuning_modified);

    obs_properties_add_bool(props, SETTING_BYPASS, TEXT_BYPASS);

    return props;
}

void defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, SETTING_PRESET, "masari_dopamine");
    obs_data_set_default_string(settings, SETTING_PRESET_APPLIED, "masari_dopamine");
    obs_data_set_default_bool(settings, SETTING_SMART, true);
    obs_data_set_default_int(settings, SETTING_MASTER, 100);
    obs_data_set_default_int(settings, SETTING_ENHANCE, 78);
    obs_data_set_default_int(settings, SETTING_COLOR_POP, 86);
    obs_data_set_default_int(settings, SETTING_CLEAN_WHITE, 72);
    obs_data_set_default_int(settings, SETTING_CLARITY, 68);
    obs_data_set_default_int(settings, SETTING_SKIN_BEAUTY, 72);
    obs_data_set_default_int(settings, SETTING_HEALTHY_TONE, 62);
    obs_data_set_default_int(settings, SETTING_TOY_GLOSS, 48);
    obs_data_set_default_int(settings, SETTING_DEPTH_POP, 76);
    obs_data_set_default_int(settings, SETTING_HIGHLIGHT_GUARD, 94);
    obs_data_set_default_string(settings, SETTING_SKIN_PROTECT, "strong");
    obs_data_set_default_string(settings, SETTING_PERFORMANCE, "ultra");
    obs_data_set_default_bool(settings, SETTING_BYPASS, false);
}

} // namespace

obs_source_info arvisual_filter_info = {};

namespace {
struct ArVisualSourceInfoInitializer {
    ArVisualSourceInfoInitializer()
    {
        arvisual_filter_info.id = "arvisual_filter";
        arvisual_filter_info.type = OBS_SOURCE_TYPE_FILTER;
        arvisual_filter_info.output_flags = OBS_SOURCE_VIDEO;
        arvisual_filter_info.get_name = filter_name;
        arvisual_filter_info.create = create;
        arvisual_filter_info.destroy = destroy;
        arvisual_filter_info.video_render = render;
        arvisual_filter_info.update = update;
        arvisual_filter_info.get_properties = properties;
        arvisual_filter_info.get_defaults = defaults;
    }
};

ArVisualSourceInfoInitializer arvisual_source_info_initializer;
} // namespace
