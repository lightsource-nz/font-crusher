#include <crush.h>
#include "crush_private.h"

#define COMMAND_RENDER                          "render"
#define COMMAND_RENDER_NEW                      "new"
#define COMMAND_RENDER_INFO                     "info"

static void print_usage_context();
static uint8_t do_cmd_render(int argc, char **argv);
static uint8_t do_cmd_render_new(int argc, char **argv);
static uint8_t do_cmd_render_info(int argc, char **argv);
uint8_t crush_cmd_render_init(struct crush_container *cmd_parent)
{
        struct crush_container *cmd_render = crush_command_container_define(
                                                        cmd_parent, COMMAND_RENDER, do_cmd_render);
        crush_command_define(cmd_render, COMMAND_RENDER_NEW, do_cmd_render_new);
        crush_command_define(cmd_render, COMMAND_RENDER_INFO, do_cmd_render_info);

        return CODE_OK;
}
static uint8_t do_cmd_render(int argc, char **argv)
{
        print_usage_context();
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

static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush render list \n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
