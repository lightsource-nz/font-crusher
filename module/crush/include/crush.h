#ifndef _CRUSH_H
#define _CRUSH_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <light.h>
#include <light_cli.h>

#include <crush_context.h>

//#include <ft2build.h>
//#include <freetype/freetype.h>

#define CODE_OK                         0
#define CODE_INVALID_ARG                1
#define CODE_NO_RESOURCE                2

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

struct crush_command {
        uint8_t type;
        char *name;
        struct crush_container *parent;
        uint8_t (*do_cmd)(int argc, char **argv);
};
struct crush_container {
        uint8_t type;
        const char *name;
        struct crush_container *parent;
        uint8_t (*do_cmd)(int argc, char **argv);
        uint8_t child_count;
        struct crush_command *child[CRUSH_SUBCOMMAND_MAX];
};
static inline struct crush_container *to_container(struct crush_command *cmd)
{
    return cmd->type == TYPE_CONTAINER ? (struct crush_container *) cmd : NULL;
}

extern struct crush_container *crush_command_container_define(struct crush_container *parent, const char *name, uint8_t (*do_cmd)(int argc, char **argv));
extern uint8_t crush_command_container_add(struct crush_container *parent, struct crush_command *child);
extern struct crush_command *crush_command_define(struct crush_container *parent, char *name, uint8_t (*do_cmd)(int argc, char **argv));
extern uint8_t crush_num_commands_defined();
extern const char *crush_command_get_name(struct crush_command *command);
extern const char *crush_command_get_path(struct crush_command *command);
extern struct crush_command *crush_command_find(struct crush_container *parent, const char *name);
extern uint8_t crush_command_exec(struct crush_command *command_id, int argc, char **argv);
extern uint8_t crush_process_command_line(int argc, char *argv[]);

extern struct crush_container *crush_command_root();
#endif
