#ifndef _CRUSH_FONT_H
#define _CRUSH_FONT_H

Light_Subcommand_Declare(cmd_crush_font, cmd_crush);
Light_Subcommand_Declare(cmd_crush_font_add, cmd_crush_font);
Light_Subcommand_Declare(cmd_crush_font_remove, cmd_crush_font);
Light_Subcommand_Declare(cmd_crush_font_info, cmd_crush_font);
Light_Subcommand_Declare(cmd_crush_font_list, cmd_crush_font);

struct crush_font {
        uint8_t *name;
        uint8_t *source_url;
};
extern uint8_t crush_font_init(struct light_command *cmd_parent);
extern struct light_command *crush_font_get_command();
extern struct light_command *crush_font_get_subcommand_add();
extern struct light_command *crush_font_get_subcommand_remove();
extern struct light_command *crush_font_get_subcommand_info();
extern struct light_command *crush_font_get_subcommand_list();

#endif
