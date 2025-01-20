#ifndef _CRUSH_FONT_H
#define _CRUSH_FONT_H

#define CRUSH_FONT_CONTEXT_OBJECT_NAME          "crush:font"
#define CRUSH_FONT_CONTEXT_JSON_FILE            "font.json"

Light_Command_Declare(cmd_crush_font, cmd_crush);
Light_Command_Declare(cmd_crush_font_add, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_remove, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_info, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_list, cmd_crush_font);

#define CRUSH_FONT_FILE_MAX                     8
struct crush_font {
        struct crush_font_context *context;
        uint8_t *name;
        uint8_t *source_url;
        uint8_t *font_type;
        uint8_t *font_version;
        uint8_t *path;
        uint8_t file_count;
        uint8_t *file[CRUSH_FONT_FILE_MAX];
};

struct crush_font_context {
        struct crush_context *root;
        uint16_t version;
        const uint8_t *file_path;
        crush_json_t *data;
};

extern uint8_t crush_font_init();
extern crush_json_t *crush_font_create_context();
extern void crush_font_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_font *crush_font_context_get(struct crush_font_context *context, const uint8_t *id);
extern uint8_t crush_font_context_save(struct crush_font_context *context, const uint8_t *id, struct crush_font *font);
extern uint8_t crush_font_context_commit(struct crush_font_context *context);
extern crush_json_t *crush_font_object_serialize(struct crush_font *font);
extern struct crush_font *crush_font_object_deserialize(crush_json_t *data);

extern struct light_command *crush_font_get_command();
extern struct light_command *crush_font_get_subcommand_add();
extern struct light_command *crush_font_get_subcommand_remove();
extern struct light_command *crush_font_get_subcommand_info();
extern struct light_command *crush_font_get_subcommand_list();

#endif
