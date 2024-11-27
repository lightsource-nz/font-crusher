#ifndef _CRUSH_FONT_H
#define _CRUSH_FONT_H
#include <light_cli.h>

struct crush_font {
        uint8_t *name;
        uint8_t *source_url;
};
extern uint8_t crush_font_init(struct light_command *cmd_parent);
extern struct light_command *crush_font_get_command();
extern struct light_command *crush_font_get_subcommand_add();
extern struct light_command *crush_font_get_subcommand_remove();

#endif
