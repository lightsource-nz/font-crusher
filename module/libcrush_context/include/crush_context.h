#ifndef _CRUSH_CONTEXT_H
#define _CRUSH_CONTEXT_H

Light_Command_Declare(cmd_crush_context, cmd_crush);
Light_Command_Declare(cmd_crush_context_new, cmd_crush_context);
Light_Command_Declare(cmd_crush_context_set, cmd_crush_context);
Light_Command_Declare(cmd_crush_context_push, cmd_crush_context);
Light_Command_Declare(cmd_crush_context_pop, cmd_crush_context);

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
