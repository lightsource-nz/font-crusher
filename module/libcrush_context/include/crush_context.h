#ifndef _CRUSH_CONTEXT_H
#define _CRUSH_CONTEXT_H

struct crush_context {
        struct crush_context *parent;
        uint8_t *path;
};

extern uint8_t crush_context_init(struct light_command *cmd_parent);

extern struct light_command *crush_context_get_command();
extern struct light_command *crush_context_get_subcommand_new();
extern struct light_command *crush_context_get_subcommand_set();
extern struct light_command *crush_context_get_subcommand_push();
extern struct light_command *crush_context_get_subcommand_pop();

#endif
