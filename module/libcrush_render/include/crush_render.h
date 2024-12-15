#ifndef _CRUSH_RENDER_H
#define _CRUSH_RENDER_H

Light_Command_Declare(cmd_crush_render, cmd_crush);
Light_Command_Declare(cmd_crush_render_new, cmd_crush_render);
Light_Command_Declare(cmd_crush_render_info, cmd_crush_render);

struct crush_render {
        uint8_t *path;
};

extern uint8_t crush_render_init(struct light_command *cmd_parent);
extern struct light_command *crush_render_get_command();
extern struct light_command *crush_render_get_subcommand_new();
extern struct light_command *crush_render_get_subcommand_info();

#endif
