#include <light.h>
#include <light_cli.h>
#include <module/mod_light_cli.h>

#include <crush_common.h>
#include <crush_module.h>

static void _event_load(const struct light_module *module)
{
        crush_module_onload();
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
Light_Module_Define(libcrush_module, _module_event,
                                                &light_cli,
                                                &light_core);
