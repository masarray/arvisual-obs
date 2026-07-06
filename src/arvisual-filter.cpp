#include <obs-module.h>
#include <graphics/vec2.h>

#include <algorithm>
#include <cstring>

#define SETTING_PRESET "preset"
#define SETTING_ENHANCE "enhance"
#define SETTING_COLOR_POP "color_pop"
#define SETTING_CLEAN_WHITE "clean_white"
#define SETTING_CLARITY "clarity"
#define SETTING_SKIN_PROTECT "skin_protect"
#define SETTING_TOY_GLOSS "toy_gloss"
#define SETTING_HIGHLIGHT_GUARD "highlight_guard"
#define SETTING_PERFORMANCE "performance"
#define SETTING_BYPASS "bypass"

#define TEXT_FILTER_NAME obs_module_text("ArVisual.FilterName")
#define TEXT_PRESET obs_module_text("ArVisual.Preset")
#define TEXT_ENHANCE obs_module_text("ArVisual.Enhance")
#define TEXT_COLOR_POP obs_module_text("ArVisual.ColorPop")
#define TEXT_CLEAN_WHITE obs_module_text("ArVisual.CleanWhite")
#define TEXT_CLARITY obs_module_text("ArVisual.VisualClarity")
#define TEXT_SKIN_PROTECT obs_module_text("ArVisual.SkinProtect")
#define TEXT_TOY_GLOSS obs_module_text("ArVisual.ToyGloss")
#define TEXT_HIGHLIGHT_GUARD obs_module_text("ArVisual.HighlightGuard")
#define TEXT_PERFORMANCE obs_module_text("ArVisual.Performance")
#define TEXT_BYPASS obs_module_text("ArVisual.Bypass")

namespace {

struct PresetValues {
    const char *id;
    const char *text;
    int enhance;
    int color_pop;
    int clean_white;
    int clarity;
    int toy_gloss;
    int highlight_guard;
    const char *skin_protect;
    const char *performance;
};

constexpr PresetValues PRESETS[] = {
    {"masari_dopamine", "Preset.MasAriDopamine", 70, 68, 55, 45, 35, 72, "normal", "standard"},
    {"kids_toy_pop", "Preset.KidsToyPop", 80, 82, 75, 48, 55, 78, "strong", "standard"},
    {"cars_candy_color", "Preset.CarsCandyColor", 85, 88, 60, 55, 65, 82, "normal", "standard"},
    {"product_clean_pop", "Preset.ProductCleanPop", 72, 65, 70, 60, 50, 76, "normal", "standard"},
    {"beauty_bright", "Preset.BeautyBright", 60, 45, 68, 28, 25, 82, "strong", "lite"},
    {"gaming_vivid", "Preset.GamingVivid", 75, 80, 35, 55, 45, 72, "normal", "standard"},
    {"natural_safe", "Preset.NaturalSafe", 38, 30, 35, 20, 15, 82, "strong", "lite"},
    {"cinematic_candy", "Preset.CinematicCandy", 65, 58, 30, 42, 35, 70, "normal", "standard"},
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
        return &PRESETS[0];

    for (const auto &preset : PRESETS) {
        if (std::strcmp(preset.id, id) == 0)
            return &preset;
    }

    return &PRESETS[0];
}

void apply_preset_to_settings(obs_data_t *settings, const PresetValues &preset)
{
    obs_data_set_int(settings, SETTING_ENHANCE, preset.enhance);
    obs_data_set_int(settings, SETTING_COLOR_POP, preset.color_pop);
    obs_data_set_int(settings, SETTING_CLEAN_WHITE, preset.clean_white);
    obs_data_set_int(settings, SETTING_CLARITY, preset.clarity);
    obs_data_set_int(settings, SETTING_TOY_GLOSS, preset.toy_gloss);
    obs_data_set_int(settings, SETTING_HIGHLIGHT_GUARD, preset.highlight_guard);
    obs_data_set_string(settings, SETTING_SKIN_PROTECT, preset.skin_protect);
    obs_data_set_string(settings, SETTING_PERFORMANCE, preset.performance);
}

bool preset_modified(obs_properties_t *, obs_property_t *, obs_data_t *settings)
{
    const auto *preset = find_preset(obs_data_get_string(settings, SETTING_PRESET));
    apply_preset_to_settings(settings, *preset);
    return true;
}

struct ArVisualFilter {
    obs_source_t *context = nullptr;
    gs_effect_t *effect = nullptr;

    gs_eparam_t *enhance_param = nullptr;
    gs_eparam_t *color_pop_param = nullptr;
    gs_eparam_t *clean_white_param = nullptr;
    gs_eparam_t *clarity_param = nullptr;
    gs_eparam_t *skin_protect_param = nullptr;
    gs_eparam_t *toy_gloss_param = nullptr;
    gs_eparam_t *highlight_guard_param = nullptr;
    gs_eparam_t *performance_param = nullptr;
    gs_eparam_t *texel_size_param = nullptr;

    float enhance = 0.7f;
    float color_pop = 0.68f;
    float clean_white = 0.55f;
    float clarity = 0.45f;
    float skin_protect = 0.65f;
    float toy_gloss = 0.35f;
    float highlight_guard = 0.72f;
    float performance = 0.5f;
    bool bypass = false;
};

const char *filter_name(void *)
{
    return TEXT_FILTER_NAME;
}

void update(void *data, obs_data_t *settings)
{
    auto *filter = static_cast<ArVisualFilter *>(data);

    filter->enhance = pct(settings, SETTING_ENHANCE);
    filter->color_pop = pct(settings, SETTING_COLOR_POP);
    filter->clean_white = pct(settings, SETTING_CLEAN_WHITE);
    filter->clarity = pct(settings, SETTING_CLARITY);
    filter->toy_gloss = pct(settings, SETTING_TOY_GLOSS);
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

    if (filter->effect) {
        obs_enter_graphics();
        gs_effect_destroy(filter->effect);
        obs_leave_graphics();
    }

    bfree(filter);
}

void *create(obs_data_t *settings, obs_source_t *context)
{
    auto *filter = static_cast<ArVisualFilter *>(bzalloc(sizeof(ArVisualFilter)));
    filter->context = context;

    char *effect_path = obs_module_file("effects/arvisual.effect");

    obs_enter_graphics();
    filter->effect = gs_effect_create_from_file(effect_path, nullptr);
    if (filter->effect) {
        filter->enhance_param = gs_effect_get_param_by_name(filter->effect, SETTING_ENHANCE);
        filter->color_pop_param = gs_effect_get_param_by_name(filter->effect, SETTING_COLOR_POP);
        filter->clean_white_param = gs_effect_get_param_by_name(filter->effect, SETTING_CLEAN_WHITE);
        filter->clarity_param = gs_effect_get_param_by_name(filter->effect, SETTING_CLARITY);
        filter->skin_protect_param = gs_effect_get_param_by_name(filter->effect, SETTING_SKIN_PROTECT);
        filter->toy_gloss_param = gs_effect_get_param_by_name(filter->effect, SETTING_TOY_GLOSS);
        filter->highlight_guard_param = gs_effect_get_param_by_name(filter->effect, SETTING_HIGHLIGHT_GUARD);
        filter->performance_param = gs_effect_get_param_by_name(filter->effect, SETTING_PERFORMANCE);
        filter->texel_size_param = gs_effect_get_param_by_name(filter->effect, "texel_size");
    }
    obs_leave_graphics();

    bfree(effect_path);

    if (!filter->effect) {
        blog(LOG_ERROR, "[ArVisual] Failed to load effects/arvisual.effect");
        destroy(filter);
        return nullptr;
    }

    update(filter, settings);
    return filter;
}

void render(void *data, gs_effect_t *effect)
{
    UNUSED_PARAMETER(effect);

    auto *filter = static_cast<ArVisualFilter *>(data);

    if (filter->bypass) {
        obs_source_skip_video_filter(filter->context);
        return;
    }

    obs_source_t *target = obs_filter_get_target(filter->context);
    const uint32_t width = target ? obs_source_get_width(target) : 0;
    const uint32_t height = target ? obs_source_get_height(target) : 0;

    if (!obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING))
        return;

    if (filter->enhance_param) gs_effect_set_float(filter->enhance_param, filter->enhance);
    if (filter->color_pop_param) gs_effect_set_float(filter->color_pop_param, filter->color_pop);
    if (filter->clean_white_param) gs_effect_set_float(filter->clean_white_param, filter->clean_white);
    if (filter->clarity_param) gs_effect_set_float(filter->clarity_param, filter->clarity);
    if (filter->skin_protect_param) gs_effect_set_float(filter->skin_protect_param, filter->skin_protect);
    if (filter->toy_gloss_param) gs_effect_set_float(filter->toy_gloss_param, filter->toy_gloss);
    if (filter->highlight_guard_param) gs_effect_set_float(filter->highlight_guard_param, filter->highlight_guard);
    if (filter->performance_param) gs_effect_set_float(filter->performance_param, filter->performance);

    if (filter->texel_size_param) {
        vec2 texel_size;
        vec2_set(&texel_size, width > 0 ? 1.0f / static_cast<float>(width) : 0.0f,
                 height > 0 ? 1.0f / static_cast<float>(height) : 0.0f);
        gs_effect_set_vec2(filter->texel_size_param, &texel_size);
    }

    gs_blend_state_push();
    gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);
    obs_source_process_filter_end(filter->context, filter->effect, 0, 0);
    gs_blend_state_pop();
}

obs_properties_t *properties(void *)
{
    obs_properties_t *props = obs_properties_create();

    obs_property_t *preset = obs_properties_add_list(props, SETTING_PRESET, TEXT_PRESET,
                                                     OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (const auto &item : PRESETS)
        obs_property_list_add_string(preset, obs_module_text(item.text), item.id);
    obs_property_set_modified_callback(preset, preset_modified);

    obs_properties_add_int_slider(props, SETTING_ENHANCE, TEXT_ENHANCE, 0, 100, 1);
    obs_properties_add_int_slider(props, SETTING_COLOR_POP, TEXT_COLOR_POP, 0, 100, 1);
    obs_properties_add_int_slider(props, SETTING_CLEAN_WHITE, TEXT_CLEAN_WHITE, 0, 100, 1);
    obs_properties_add_int_slider(props, SETTING_CLARITY, TEXT_CLARITY, 0, 100, 1);
    obs_properties_add_int_slider(props, SETTING_TOY_GLOSS, TEXT_TOY_GLOSS, 0, 100, 1);
    obs_properties_add_int_slider(props, SETTING_HIGHLIGHT_GUARD, TEXT_HIGHLIGHT_GUARD, 0, 100, 1);

    obs_property_t *skin = obs_properties_add_list(props, SETTING_SKIN_PROTECT, TEXT_SKIN_PROTECT,
                                                   OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(skin, obs_module_text("Skin.Off"), "off");
    obs_property_list_add_string(skin, obs_module_text("Skin.Normal"), "normal");
    obs_property_list_add_string(skin, obs_module_text("Skin.Strong"), "strong");

    obs_property_t *performance = obs_properties_add_list(props, SETTING_PERFORMANCE, TEXT_PERFORMANCE,
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(performance, obs_module_text("Performance.Lite"), "lite");
    obs_property_list_add_string(performance, obs_module_text("Performance.Standard"), "standard");
    obs_property_list_add_string(performance, obs_module_text("Performance.Ultra"), "ultra");

    obs_properties_add_bool(props, SETTING_BYPASS, TEXT_BYPASS);

    return props;
}

void defaults(obs_data_t *settings)
{
    obs_data_set_default_string(settings, SETTING_PRESET, "masari_dopamine");
    obs_data_set_default_int(settings, SETTING_ENHANCE, 70);
    obs_data_set_default_int(settings, SETTING_COLOR_POP, 68);
    obs_data_set_default_int(settings, SETTING_CLEAN_WHITE, 55);
    obs_data_set_default_int(settings, SETTING_CLARITY, 45);
    obs_data_set_default_int(settings, SETTING_TOY_GLOSS, 35);
    obs_data_set_default_int(settings, SETTING_HIGHLIGHT_GUARD, 72);
    obs_data_set_default_string(settings, SETTING_SKIN_PROTECT, "normal");
    obs_data_set_default_string(settings, SETTING_PERFORMANCE, "standard");
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
