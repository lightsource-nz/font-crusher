#ifndef _CRUSH_MODULE_H
#define _CRUSH_MODULE_H

struct crush_module {
        struct crush_module *parent;
        uint8_t *path;
};

extern struct light_command *crush_get_command_module();
extern struct light_command *crush_get_command_module_push();
extern struct light_command *crush_get_command_module_get();
extern struct light_command *crush_get_command_module_define();

#endif
