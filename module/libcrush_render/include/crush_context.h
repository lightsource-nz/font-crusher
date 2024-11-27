#ifndef _CRUSH_CONTEXT_H
#define _CRUSH_CONTEXT_H

struct crush_context {
        struct crush_context *parent;
        uint8_t *path;
};

extern struct light_command *crush_context_get_subcommand_push();
extern struct light_command *crush_context_get_subcommand_get();
extern struct light_command *crush_context_get_subcommand_define();

extern struct crush_container *crush_command_context();
#endif
