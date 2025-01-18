#ifndef _CRUSH_MODULE_H
#define _CRUSH_MODULE_H

#define CRUSH_MODULE_CONTEXT_OBJECT_NAME          "crush:module"
#define CRUSH_MODULE_CONTEXT_JSON_FILE            "module.json"

Light_Command_Declare(cmd_crush_module, cmd_crush);
Light_Command_Declare(cmd_crush_module_info, cmd_crush_module);
Light_Command_Declare(cmd_crush_module_import, cmd_crush_module);

struct crush_module {
        struct crush_module *parent;
        uint8_t *path;
};

extern uint8_t crush_module_init(struct light_command *cmd_parent);
extern struct crush_json crush_module_create_context();
extern void crush_module_load_context(struct crush_context *context, struct crush_json data);

extern struct light_command *crush_get_command_module();
extern struct light_command *crush_get_command_module_info();
extern struct light_command *crush_get_command_module_import();

#endif
