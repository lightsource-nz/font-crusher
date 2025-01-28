#ifndef _CRUSH_RENDER_H
#define _CRUSH_RENDER_H

#define CRUSH_RENDER_CONTEXT_OBJECT_NAME        "crush:render"
#define CRUSH_RENDER_CONTEXT_JSON_FILE          "render.json"

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
        uint32_t next_id;
        crush_json_t *data;
};

struct crush_render {
        uint8_t *name;
        struct crush_font *font;
        struct crush_display *display;
        uint8_t *path;
};

extern uint8_t crush_render_init(struct light_command *cmd_parent);
// render-context API
extern struct crush_render_context *crush_render_context();
extern struct crush_render_context *crush_render_get_context(struct crush_context *root);
extern crush_json_t *crush_render_create_context(uint8_t *path);
extern void crush_render_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_render *crush_render_context_get(struct crush_render_context *context, const uint8_t *id);
extern uint8_t crush_render_context_save(struct crush_render_context *context, const uint8_t *id, struct crush_render *object);
extern uint8_t crush_render_context_commit(struct crush_render_context *context);
extern uint8_t *crush_render_context_new_id(struct crush_render_context *context);

extern crush_json_t *crush_render_object_serialize(struct crush_render *font);
extern struct crush_render *crush_render_object_deserialize(crush_json_t *data);
extern struct crush_font *crush_render_get_font(struct crush_render *render);
extern void crush_render_set_font(struct crush_render *render, struct crush_font *font);
extern struct crush_display *crush_render_get_display(struct crush_render *render);
extern void crush_render_set_display(struct crush_render *render, struct crush_display *display);

extern struct light_command *crush_render_get_command();
extern struct light_command *crush_render_get_subcommand_new();
extern struct light_command *crush_render_get_subcommand_info();

#endif
