#ifndef _CRUSH_RENDER_H
#define _CRUSH_RENDER_H

struct crush_render {
        uint8_t *path;
};

extern uint8_t crush_render_init(struct light_command *cmd_parent);
extern struct light_command *crush_render_get_command();
extern struct light_command *crush_render_get_subcommand_new();
extern struct light_command *crush_render_get_subcommand_info();

#endif
