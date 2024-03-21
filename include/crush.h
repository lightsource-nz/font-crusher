#ifndef _CRUSH_H
#define _CRUSH_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#define CODE_OK                         0
#define CODE_INVALID_ARG                1

#define UNIT_PIXEL                      0
#define UNIT_POINT                      1
#define DPI_H_DEFAULT                   300
#define DPI_V_DEFAULT                   300

#define ID_COMMAND_CONTEXT                 1
#define ID_COMMAND_DISPLAY                 2
#define ID_COMMAND_FONT                    3
#define ID_COMMAND_MODULE                  4
#define ID_COMMAND_RENDER                  5

#define COMMAND_INVALID                 0

#define STR_COMMAND_CONTEXT             "context"
#define STR_COMMAND_DISPLAY             "display"
#define STR_COMMAND_FONT                "font"
#define STR_COMMAND_MODULE              "module"
#define STR_COMMAND_RENDER              "render"

// defines the maximum number of command handles, including containers
#define CRUSH_COMMAND_MAX               32
#define CRUSH_SUBCOMMAND_MAX            8
#define CRUSH_COMMAND_PARENT_NONE       0

#define TYPE_COMMAND                    0
#define TYPE_CONTAINER                  1

uint8_t command_context();
uint8_t command_display();
uint8_t command_font();
uint8_t command_module();
uint8_t command_render();

uint8_t crush_command_container_define(uint8_t parent, const char *name);
uint8_t crush_command_define(uint8_t parent, const char *name, uint8_t (*do_cmd)(int argc, char **argv));
uint8_t crush_num_commands_defined();
const char *crush_command_get_name(uint8_t command_id);
uint8_t crush_command_find(uint8_t parent, const char *name);
uint8_t crush_command_exec(uint8_t command_id, int argc, char **argv);

#endif
