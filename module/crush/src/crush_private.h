#ifndef _CRUSH_PRIVATE_H
#define _CRUSH_PRIVATE_H

uint8_t crush_cmd_context_init(struct light_command *cmd_parent);
uint8_t crush_cmd_display_init(struct light_command *cmd_parent);
uint8_t crush_cmd_font_init(struct light_command *cmd_parent);
uint8_t crush_cmd_module_init(struct light_command *cmd_parent);
uint8_t crush_cmd_render_init(struct light_command *cmd_parent);

#endif
