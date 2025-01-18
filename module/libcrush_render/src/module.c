#include <light.h>
#include <module/mod_crush_context.h>
#include <module/mod_light_cli.h>
#include <module/mod_freetype.h>

#include <light_cli.h>
#include <crush_common.h>
#include <crush_render.h>

#include "render_private.h"

static void _event_load(const struct light_module *module)
{
        _render_load_event();
        crush_common_register_context_object_loader(CRUSH_RENDER_CONTEXT_OBJECT_NAME, CRUSH_RENDER_CONTEXT_JSON_FILE, 
                                        crush_render_create_context, crush_render_load_context);
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
Light_Module_Define(libcrush_render, _module_event,
                                                &mod_freetype,
                                                &light_cli,
                                                &light_core);
