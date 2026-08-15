#ifndef _CRUSH_RENDER_H
#define _CRUSH_RENDER_H

#define CRUSH_RENDER_CONTEXT_OBJECT_NAME        "crush:render"
#define CRUSH_RENDER_CONTEXT_JSON_FILE          "render.json"

#define CRUSH_RENDER_STATE_NEW                  0
#define CRUSH_RENDER_STATE_RUNNING              1
#define CRUSH_RENDER_STATE_DONE                 2
#define CRUSH_RENDER_STATE_PAUSE                3
#define CRUSH_RENDER_STATE_CANCEL               4
#define CRUSH_RENDER_STATE_ERROR                5

#define CRUSH_RENDER_CONTEXT_SUBDIR_NAME        "render"

#define CRUSH_RENDER_CALLBACK_SIGNAL            SIGUSR1

Light_Command_Declare(cmd_crush_render, cmd_crush);
Light_Command_Declare(cmd_crush_render_new, cmd_crush_render);
Light_Command_Declare(cmd_crush_render_info, cmd_crush_render);
Light_Command_Declare(cmd_crush_render_list, cmd_crush_render);

Light_Command_Option_Declare(opt_crush_render_new_font, &cmd_crush_render_new);
Light_Command_Option_Declare(opt_crush_render_new_display, &cmd_crush_render_new);

struct crush_render_context {
        light_mutex_t lock;
        struct crush_context *root;
        const uint8_t *file_path;
        uint16_t version;
        atomic_uint_least32_t next_id;
        //   `data` is the sub-object taken with an incref by the O in CONTEXT_OBJECT_FMT.
        // `data_root` is the whole document the loader handed over -- the loader becomes its
        // owner, so it has to be kept to be released, or the document leaks
        crush_json_t *data;
        crush_json_t *data_root;
};

struct crush_render {
        struct crush_render_context *context;
        crush_json_t *data;
        _Atomic uint32_t id;
        struct render_job *render_job;
        uint8_t *name;
        _Atomic uint8_t state;
        struct crush_font *font;
        uint8_t font_size;
        uint8_t pixel_size;
        struct crush_display *display;
        uint8_t *path;
        //   _Atomic: the worker publishes this with atomic_store() from its own thread. Same
        // rule as id and state above -- the operand of an atomic operation has to be an atomic
        // object, and a pointer is no exception
        _Atomic(uint8_t **) output;
        //   set once the worker thread has finished with this render ENTIRELY -- after its
        // saves, after the context commit, after every touch of shared state. Runtime only:
        // never serialised, and deliberately separate from `state` above.
        //
        //   `state` cannot serve this purpose even though it reaches DONE, because the polling
        // loop calls crush_render_refresh(), which reloads the object from the context JSON and
        // therefore overwrites whatever is in memory with whatever was last persisted. So state
        // has to be stored BEFORE the save that persists it, which is necessarily before the
        // worker has finished -- and a foreground thread released at that moment goes on to
        // tear down the context this thread is still writing into
        atomic_bool complete;
};

// render-context API
extern struct crush_render_context *crush_render_context(void);
extern struct crush_render_context *crush_render_get_context(struct crush_context *root);
// the `uint8_t *path` this used to declare was never passed and never read. it is registered
// as a context-object create callback and invoked as create() with no arguments at all (see
// crush_common_create_context()), so under C17's "unspecified parameters" it merely received
// whatever happened to be in the argument register; under C23 the mismatch is a hard error.
// the other three create callbacks -- display, font, module -- have always taken none
extern crush_json_t *crush_render_create_context(void);
extern void crush_render_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern void crush_render_destroy_context(struct crush_render_context *context);
extern struct crush_render *crush_render_context_get(struct crush_render_context *context, const uint32_t id);
extern struct crush_render *crush_render_context_get_by_name(struct crush_render_context *context, const uint8_t *name);
static inline struct crush_render *crush_render_get(const uint32_t id)
{
        return crush_render_context_get(crush_render_context(), id);
}
static inline struct crush_render *crush_render_get_by_name(const uint8_t *name)
{
        return crush_render_context_get_by_name(crush_render_context(), name);
}
extern uint8_t crush_render_context_save(struct crush_render_context *context, struct crush_render *object);
extern uint8_t crush_render_context_refresh(struct crush_render_context *context, struct crush_render *object);
extern uint8_t crush_render_context_commit(struct crush_render_context *context);
static inline uint8_t crush_render_save(struct crush_render *object)
{
        return crush_render_context_save(object->context, object);
}
static inline uint8_t crush_render_refresh(struct crush_render *object)
{
        return crush_render_context_refresh(object->context, object);
}
static inline uint8_t crush_render_commit(void)
{
        return crush_render_context_commit(crush_render_context());
}

extern crush_json_t *crush_render_object_serialize(struct crush_render *object);
extern void crush_render_object_extract(crush_json_t *data, struct crush_render *object);
extern struct crush_render *crush_render_object_deserialize(crush_json_t *data);

extern void crush_render_module_load(void);
extern void crush_render_module_unload(void);

extern uint8_t *crush_render_context_get_root_path(struct crush_render_context *context);
extern void crush_render_init_ctx(struct crush_render_context *context, struct crush_render *render, const uint8_t *name, struct crush_font *font, uint8_t font_size, uint8_t pixel_size, struct crush_display *display);
extern void crush_render_init(struct crush_render *render, const uint8_t *name, struct crush_font *font, uint8_t font_size, uint8_t pixel_size, struct crush_display *display);
extern void crush_render_release(struct crush_render *render);
extern uint32_t crush_render_get_id(struct crush_render *render);
extern uint8_t crush_render_get_state(struct crush_render *render);
extern uint8_t *crush_render_get_name(struct crush_render *render);
extern struct crush_font *crush_render_get_font(struct crush_render *render);
extern void crush_render_set_font(struct crush_render *render, struct crush_font *font);
extern uint8_t crush_render_get_font_size(struct crush_render *render);
extern void crush_render_set_font_size(struct crush_render *render, uint8_t font_size);
extern uint8_t crush_render_get_pixel_size(struct crush_render *render);
extern void crush_render_set_pixel_size(struct crush_render *render, uint8_t pixel_size);
extern struct crush_display *crush_render_get_display(struct crush_render *render);
extern void crush_render_set_display(struct crush_render *render, struct crush_display *display);

extern uint8_t crush_render_start_render_job(struct crush_render *render);
extern uint8_t crush_render_cancel_render_job(struct crush_render *render);
extern uint8_t crush_render_complete_render_job(struct crush_render *render);
extern uint8_t crush_render_fail_render_job(struct crush_render *render);

#endif
