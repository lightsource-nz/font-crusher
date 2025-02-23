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

Light_Command_Declare(cmd_crush_render, cmd_crush);
Light_Command_Declare(cmd_crush_render_new, cmd_crush_render);
Light_Command_Declare(cmd_crush_render_info, cmd_crush_render);
Light_Command_Declare(cmd_crush_render_list, cmd_crush_render);

Light_Command_Option_Declare(opt_crush_render_new_font, &cmd_crush_render_new);
Light_Command_Option_Declare(opt_crush_render_new_display, &cmd_crush_render_new);

struct crush_render_context {
        const struct crush_context *root;
        const uint8_t *file_path;
        uint16_t version;
        atomic_uint_least32_t next_id;
        crush_json_t *data;
};

struct crush_render {
        struct crush_render_context *context;
        uint32_t id;
        uint8_t job_id;
        uint8_t *name;
        uint8_t state;
        struct crush_font *font;
        uint8_t font_size;
        struct crush_display *display;
        uint8_t *path;
        uint8_t **output;
};

// render-context API
extern struct crush_render_context *crush_render_context();
extern struct crush_render_context *crush_render_get_context(struct crush_context *root);
extern crush_json_t *crush_render_create_context(uint8_t *path);
extern void crush_render_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
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
extern uint8_t crush_render_context_commit(struct crush_render_context *context);
static inline uint8_t crush_render_save(struct crush_render *object)
{
        return crush_render_context_save(object->context, object);
}
static inline uint8_t crush_render_commit()
{
        return crush_render_context_commit(crush_render_context());
}

extern crush_json_t *crush_render_object_serialize(struct crush_render *font);
extern struct crush_render *crush_render_object_deserialize(crush_json_t *data);

extern void crush_render_module_load();
extern void crush_render_module_unload();

extern void crush_render_init(struct crush_render *render, struct crush_font *font, uint8_t font_size, struct crush_display *display, const uint8_t *name);
extern void crush_render_release(struct crush_render *render);
extern uint32_t crush_render_get_id(struct crush_render *render);
extern uint8_t crush_render_get_state(struct crush_render *render);
extern uint8_t *crush_render_get_name(struct crush_render *render);
extern struct crush_font *crush_render_get_font(struct crush_render *render);
extern void crush_render_set_font(struct crush_render *render, struct crush_font *font);
extern uint8_t crush_render_get_font_size(struct crush_render *render);
extern void crush_render_set_font_size(struct crush_render *render, uint8_t font_size);
extern struct crush_display *crush_render_get_display(struct crush_render *render);
extern void crush_render_set_display(struct crush_render *render, struct crush_display *display);

extern uint8_t crush_render_start_render_job(struct crush_render *render);
extern uint8_t crush_render_cancel_render_job(struct crush_render *render);

extern struct light_command *crush_render_get_command();
extern struct light_command *crush_render_get_subcommand_new();
extern struct light_command *crush_render_get_subcommand_info();

#endif
