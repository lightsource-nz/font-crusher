#ifndef _CRUSH_MODULE_H
#define _CRUSH_MODULE_H

#define CRUSH_MODULE_CONTEXT_OBJECT_NAME          "crush:module"
#define CRUSH_MODULE_CONTEXT_JSON_FILE            "module.json"

Light_Command_Declare(cmd_crush_module, cmd_crush);
Light_Command_Declare(cmd_crush_module_info, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_import, cmd_crush_module);

struct crush_module_context {
        struct crush_context *root;
        uint16_t version;
        const uint8_t *file_path;
        crush_json_t *data;
};

struct crush_module {
        struct crush_module *parent;
        uint8_t *path;
};

extern uint8_t crush_module_init(struct light_command *cmd_parent);
extern crush_json_t *crush_module_create_context();
extern void crush_module_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data);
extern struct crush_module *crush_module_context_get(struct crush_module_context *context, const uint8_t *id);
extern uint8_t crush_module_context_save(struct crush_module_context *context, const uint8_t *id, struct crush_module *font);
extern uint8_t crush_module_context_commit(struct crush_module_context *context);

extern crush_json_t crush_module_object_serialize(struct crush_module *font);
extern struct crush_module *crush_module_object_deserialize(crush_json_t data);

extern struct light_command *crush_get_command_module();
extern struct light_command *crush_get_command_module_info();
extern struct light_command *crush_get_command_module_import();

#endif
