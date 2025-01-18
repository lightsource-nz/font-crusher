#include <light.h>
#include <module/mod_crush_common.h>
#include <module/mod_jansson.h>
#include <crush_common.h>

static void _event_load(const struct light_module *module)
{
        crush_common_init();
}
static void _event_launch()
{
        crush_common_load_context();
}
static void _event_unload(const struct light_module *module)
{
        
}
static void _module_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:
                _event_load(module);
                break;
        case LF_EVENT_APP_LAUNCH:
                _event_launch();
                break;
        case LF_EVENT_MODULE_UNLOAD:
                _event_unload(module);
                break;
        }
}
Light_Module_Define(libcrush_common, _module_event,
                                                &mod_jansson,
                                                &light_core);
