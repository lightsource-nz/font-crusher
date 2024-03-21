#include <crush.h>
#include "crush_private.h"

#define COMMAND_RENDER                          "render"
#define COMMAND_RENDER_NEW                      "new"
#define COMMAND_RENDER_INFO                     "info"

static void print_usage();
static uint8_t do_cmd_render_new(int argc, char **argv);
static uint8_t do_cmd_render_info(int argc, char **argv);
uint8_t crush_cmd_render_init()
{
        uint8_t id_render = crush_command_container_define(
                                CRUSH_COMMAND_PARENT_NONE, COMMAND_RENDER);
        crush_command_define(id_render, COMMAND_RENDER_NEW, do_cmd_render_new);
        crush_command_define(id_render, COMMAND_RENDER_INFO, do_cmd_render_info);

        return CODE_OK;
}
static uint8_t do_cmd_render_new(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_render_info(int argc, char **argv)
{
        return CODE_OK;
}

static void print_usage()
{
        printf(
                "Usage:\n"
                "crush render list \n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
