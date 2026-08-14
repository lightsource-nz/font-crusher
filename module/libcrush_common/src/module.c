#include <light.h>
#include <module/mod_crush_common.h>
#include <module/mod_jansson.h>
#include <crush_common.h>

static void _event_load(const struct light_module *module)
{
        crush_common_init();
}
static void _event_launch(void)
{
        crush_common_load_context();
}
//   deliberately empty, which is not the same thing as unfinished.
//
//   crush_common_init()'s one effect is json_set_alloc_funcs(light_alloc, light_free), and
// that must NOT be undone here. It is a process-global that has to stay consistent for as long
// as any json object is alive: every object allocated through light_alloc must be freed through
// light_free. This module unloads before mod_jansson, so restoring the default allocators here
// would hand jansson's own teardown a mismatched free() and corrupt the heap.
//
//   the loader table this module owns is emptied by its registrants, each withdrawing its own
// entry through crush_common_unregister_context_object_loader() as it unloads
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
