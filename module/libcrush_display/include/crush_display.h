#ifndef _CRUSH_DISPLAY_H
#define _CRUSH_DISPLAY_H

struct crush_display {
        uint8_t *name;
        uint8_t *description;
};

extern struct light_command *crush_display_get_command();
extern struct light_command *crush_display_get_subcommand_push();
extern struct light_command *crush_display_get_subcommand_get();
extern struct light_command *crush_display_get_subcommand_define();

#endif
