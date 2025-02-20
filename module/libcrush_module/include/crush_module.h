#ifndef _CRUSH_MODULE_H
#define _CRUSH_MODULE_H

#define CRUSH_MODULE_CONTEXT_OBJECT_NAME          "crush:module"
#define CRUSH_MODULE_CONTEXT_JSON_FILE            "module.json"

Light_Command_Declare(cmd_crush_module, cmd_crush);
Light_Command_Declare(cmd_crush_module_info, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_list, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_add, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_remove, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_load, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_unload, cmd_crush_module);

struct crush_module_context {
        struct crush_context *root;
        uint16_t version;
        atomic_uint_least32_t next_id;
        const uint8_t *file_path;
        crush_json_t *data;
};

#define CRUSH_MODULE_FILE_MAX           8
#define CRUSH_MODULE_DEPS_MAX           16
struct crush_module {
        struct crush_module_context *context;
        uint32_t id;
        uint8_t *name;
        uint8_t *version_str;
        struct crush_module *parent;
        uint8_t *source_url;
        uint8_t *path;
        uint8_t deps_count;
        uint8_t *deps[CRUSH_MODULE_DEPS_MAX];
        uint8_t file_count;
        uint8_t *file[CRUSH_MODULE_FILE_MAX];
};

extern uint8_t *crush_module_get_name(struct crush_module *module);
extern uint8_t *crush_module_get_version_str(struct crush_module *module);
extern struct crush_module *crush_module_get_parent(struct crush_module *module);
extern uint8_t *crush_module_get_source_url(struct crush_module *module);
extern uint8_t *crush_module_get_path(struct crush_module *module);
extern uint8_t crush_module_get_deps_count(struct crush_module *module);
extern uint8_t *crush_module_get_deps(struct crush_module *module, uint8_t index);
extern uint8_t crush_module_get_file_count(struct crush_module *module);
extern uint8_t *crush_module_get_file(struct crush_module *module, uint8_t index);

extern uint8_t crush_module_init();
extern crush_json_t *crush_module_create_context();
extern void crush_module_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_module *crush_module_context_get(struct crush_module_context *context, const uint8_t *id);
extern uint8_t crush_module_context_save(struct crush_module_context *context, struct crush_module *module);
extern uint8_t crush_module_context_commit(struct crush_module_context *context);

extern crush_json_t *crush_module_object_serialize(struct crush_module *font);
extern struct crush_module *crush_module_object_deserialize(crush_json_t *data);

static inline struct light_command *crush_get_command_module()
{
        return &cmd_crush_module;
}
static inline struct light_command *crush_get_command_module_info()
{
        return &cmd_crush_module_info;
}
static inline struct light_command *crush_get_command_module_add()
{
        return &cmd_crush_module_add;
}
static inline struct light_command *crush_get_command_module_remove()
{
        return &cmd_crush_module_remove;
}
static inline struct light_command *crush_get_command_module_load()
{
        return &cmd_crush_module_load;
}
static inline struct light_command *crush_get_command_module_unload()
{
        return &cmd_crush_module_unload;
}

#endif
