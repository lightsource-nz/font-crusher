#include <light.h>
#include <module/mod_freetype.h>
#include <module/mod_crush_render_backend.h>

#include <crush_common.h>
#include <crush_render_backend.h>

static void _event_load(const struct light_module *module)
{
        render_backend_init();
}
static void _event_unload(const struct light_module *module)
{
        render_backend_shutdown();
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
Light_Module_Define(crush_render_backend, _module_event,
                                                &mod_freetype,
                                                &light_core);
