#include <light.h>
#include <module/mod_light_cli.h>
#include <module/mod_crush_context.h>
#include <module/mod_crush_display.h>
#include <module/mod_crush_font.h>
#include <module/mod_crush_module.h>
#include <module/mod_crush_render.h>
#include <module/mod_freetype.h>

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
Light_Module_Define(libcrush, _module_event,
                                                &libcrush_context,
                                                &libcrush_display,
                                                &libcrush_font,
                                                &libcrush_module,
                                                &libcrush_render,
                                                &light_cli,
                                                &light_core);
