#ifndef _CRUSH_DISPLAY_H
#define _CRUSH_DISPLAY_H

#define CRUSH_DISPLAY_CONTEXT_OBJECT_NAME       "crush:display"
#define CRUSH_DISPLAY_CONTEXT_JSON_FILE         "display.json"

Light_Command_Declare(cmd_crush_display, cmd_crush);
Light_Command_Declare(cmd_crush_display_import, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_info, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_remove, cmd_crush_display);

struct crush_display {
        uint8_t *name;
        uint8_t *description;
        uint16_t resolution_h;
        uint16_t resolution_v;
        uint16_t ppi_h;
        uint16_t ppi_v;
        double height_mm;
        double width_mm;
};

struct crush_display_context {
        struct crush_context *root;
        uint16_t version;
        crush_json_t *data;
        const uint8_t *file_path;
        struct crush_display *(*get)(uint8_t name);
};

extern uint8_t crush_display_init();
extern crush_json_t *crush_display_create_context();
extern void crush_display_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *json);
extern struct crush_display *crush_display_context_get(struct crush_display_context *context, const uint8_t *id);
extern uint8_t crush_display_context_save(struct crush_display_context *context, const uint8_t *id, struct crush_display *object);
extern uint8_t crush_display_context_commit(struct crush_display_context *context);

extern crush_json_t *crush_display_object_serialize(struct crush_display *font);
extern struct crush_display *crush_display_object_deserialize(crush_json_t *data);

#define crush_display_get_context_object(_context) \
        crush_context_get_context_object_type(_context, CRUSH_DISPLAY_CONTEXT_OBJECT_NAME, struct )

extern struct light_command *crush_display_get_command();
extern struct light_command *crush_display_get_subcommand_import();
extern struct light_command *crush_display_get_subcommand_info();

#endif
