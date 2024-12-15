#ifndef _CRUSH_DISPLAY_H
#define _CRUSH_DISPLAY_H

Light_Command_Declare(cmd_crush_display, cmd_crush);
Light_Command_Declare(cmd_crush_display_import, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_info, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_remove, cmd_crush_display);

struct crush_display {
        uint8_t *name;
        uint8_t *description;
};

extern uint8_t crush_display_init(struct light_command *cmd_parent);

extern struct light_command *crush_display_get_command();
extern struct light_command *crush_display_get_subcommand_import();
extern struct light_command *crush_display_get_subcommand_info();

#endif
