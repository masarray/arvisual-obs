#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("arvisual", "en-US")

extern obs_source_info arvisual_filter_info;

bool obs_module_load(void)
{
    obs_register_source(&arvisual_filter_info);
    blog(LOG_INFO, "[ArVisual] Smart Color Enhancer loaded");
    return true;
}

void obs_module_unload(void)
{
    blog(LOG_INFO, "[ArVisual] Smart Color Enhancer unloaded");
}
