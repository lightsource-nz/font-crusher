#ifndef _CRUSH_FONT_H
#define _CRUSH_FONT_H

#define CRUSH_FONT_CONTEXT_OBJECT_NAME          "crush:font"
#define CRUSH_FONT_CONTEXT_JSON_FILE            "font.json"

Light_Command_Declare(cmd_crush_font, cmd_crush);
Light_Command_Declare(cmd_crush_font_add, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_remove, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_info, cmd_crush_font);
Light_Command_Declare(cmd_crush_font_list, cmd_crush_font);

Light_Command_Option_Declare(cmd_crush_font_add__opt_localfile, cmd_crush_font_add);

#define CRUSH_FONT_FILE_MAX                     8
struct crush_font {
        uint32_t id;
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
        uint32_t next_id;
        crush_json_t *data;
};

extern uint8_t crush_font_init();
extern struct crush_font_context *crush_font_context();
extern struct crush_font_context *crush_font_get_context(struct crush_context *root);
extern crush_json_t *crush_font_create_context();
extern void crush_font_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_font *crush_font_context_get(struct crush_font_context *context, const uint32_t id);
extern struct crush_font *crush_font_context_get_by_name(struct crush_font_context *context, const uint8_t *name);
static inline struct crush_font *crush_font_get(const uint32_t id)
{
        return crush_font_context_get(crush_font_context(), id);
}
static inline struct crush_font *crush_font_get_by_name(const uint8_t *name)
{
        return crush_font_context_get_by_name(crush_font_context(), name);
}
extern uint8_t crush_font_context_save(struct crush_font_context *context, struct crush_font *font);
extern uint8_t crush_font_context_commit(struct crush_font_context *context);
extern crush_json_t *crush_font_object_serialize(struct crush_font *font);
extern struct crush_font *crush_font_object_deserialize(crush_json_t *data);

extern uint32_t crush_font_get_id(struct crush_font *font);
extern uint8_t *crush_font_get_name(struct crush_font *font);
// these accessors determine the specific font file and index used to render this font definition
extern uint8_t *crush_font_get_target_file(struct crush_font *font);
extern uint8_t crush_font_get_target_face_index(struct crush_font *font);

static inline struct light_command *crush_font_get_command()
{
        return &cmd_crush_font;
}
static inline struct light_command *crush_font_get_subcommand_add()
{
        return &cmd_crush_font_add;
}
static inline struct light_command *crush_font_get_subcommand_remove()
{
        return &cmd_crush_font_remove;
}
static inline struct light_command *crush_font_get_subcommand_info()
{
        return &cmd_crush_font_info;
}
static inline struct light_command *crush_font_get_subcommand_list()
{
        return &cmd_crush_font_list;
}

#endif
