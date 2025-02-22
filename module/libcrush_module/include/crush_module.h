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

#define CRUSH_MODULE_STATE_NEW          0
#define CRUSH_MODULE_STATE_UPDATING     1
#define CRUSH_MODULE_STATE_READY        2
#define CRUSH_MODULE_STATE_LOADING      3
#define CRUSH_MODULE_STATE_ACTIVE       4
#define CRUSH_MODULE_STATE_ERROR        5

#define CRUSH_MODULE_FILE_MAX           8
#define CRUSH_MODULE_DEPS_MAX           16
struct crush_module {
        struct crush_module_context *context;
        uint32_t id;
        atomic_uchar state;
        const uint8_t *name;
        uint16_t version_seq;
        const uint8_t *version_str;
        struct crush_module *parent;
        const uint8_t *source_url;
        const uint8_t *path;
        uint8_t deps_count;
        struct crush_module *deps[CRUSH_MODULE_DEPS_MAX];
        uint8_t file_count;
        uint8_t *file[CRUSH_MODULE_FILE_MAX];
};

extern const uint8_t *crush_module_get_name(struct crush_module *module);
extern const uint8_t *crush_module_get_version_str(struct crush_module *module);
extern struct crush_module *crush_module_get_parent(struct crush_module *module);
extern const uint8_t *crush_module_get_source_url(struct crush_module *module);
extern const uint8_t *crush_module_get_path(struct crush_module *module);
extern uint8_t crush_module_get_deps_count(struct crush_module *module);
extern struct crush_module *crush_module_get_deps(struct crush_module *module, uint8_t index);
extern uint8_t crush_module_get_file_count(struct crush_module *module);
extern uint8_t *crush_module_get_file(struct crush_module *module, uint8_t index);
//   NOTE crush_module_has_direct_dependency() checks only the deps list of the given module.
// to check recursively for an indirect dependency relationship, use crush_module_depends_on()
extern bool crush_module_has_direct_dependency(struct crush_module *module, struct crush_module *other);
extern bool crush_module_depends_on(struct crush_module *module, struct crush_module *other);

extern void crush_module_init(struct crush_module *module, struct crush_module_context *context, struct crush_module *parent, const uint8_t *name, const uint16_t version_seq, const uint8_t *source_url, const uint8_t *path);
extern void crush_module_release(struct crush_module *module);
// list accessors return LIGHT_NO_RESOURCE on list overflow
extern uint8_t crush_module_add_dependency(struct crush_module *module, struct crush_module *dependency);
extern uint8_t crush_module_add_file(struct crush_module *module, uint8_t *filename);

extern uint8_t crush_module_onload();
extern struct crush_module_context *crush_module_context();
extern struct crush_module_context *crush_module_get_context(struct crush_context *root);
extern crush_json_t *crush_module_create_context();
extern void crush_module_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_module *crush_module_context_get(struct crush_module_context *context, const uint32_t id);
extern struct crush_module *crush_module_context_get_by_name(struct crush_module_context *context, const uint8_t *name);
static inline struct crush_module *crush_module_get(const uint32_t id)
{
        return crush_module_context_get(crush_module_context(), id);
}
static inline struct crush_module *crush_module_get_by_name(const uint8_t *name)
{
        return crush_module_context_get_by_name(crush_module_context(), name);
}
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
