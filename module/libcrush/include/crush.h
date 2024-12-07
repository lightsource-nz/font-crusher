#ifndef _CRUSH_H
#define _CRUSH_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <light.h>
#include <light_cli.h>

Light_Command_Declare(cmd_crush);

#include <crush_context.h>
#include <crush_display.h>
#include <crush_font.h>
#include <crush_module.h>
#include <crush_render.h>

//#include <ft2build.h>
//#include <freetype/freetype.h>

#define CODE_OK                         0
#define CODE_INVALID_ARG                1
#define CODE_NO_RESOURCE                2

#define UNIT_PIXEL                      0
#define UNIT_POINT                      1
#define DPI_H_DEFAULT                   300
#define DPI_V_DEFAULT                   300

extern uint8_t crush_process_command_line(int argc, char *argv[]);

extern struct light_command *crush_command_root();
#endif
