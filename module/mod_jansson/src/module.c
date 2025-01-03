#include <light.h>
#include <module/mod_jansson.h>

static void _event_load(const struct light_module *module)
{
        
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
        case LF_EVENT_MODULE_UNLOAD:
                _event_unload(module);
                break;
        }
}
Light_Module_Define(mod_jansson, _module_event,
                                                &light_core);
