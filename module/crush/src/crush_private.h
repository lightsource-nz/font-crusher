#ifndef _CRUSH_PRIVATE_H
#define _CRUSH_PRIVATE_H

uint8_t crush_cmd_context_init(struct crush_container *cmd_parent);
uint8_t crush_cmd_display_init(struct crush_container *cmd_parent);
uint8_t crush_cmd_font_init(struct crush_container *cmd_parent);
uint8_t crush_cmd_module_init(struct crush_container *cmd_parent);
uint8_t crush_cmd_render_init(struct crush_container *cmd_parent);

#endif
