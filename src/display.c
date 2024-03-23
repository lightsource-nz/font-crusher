#include <crush.h>
#include "crush_private.h"

#define COMMAND_DISPLAY                         "display"
#define COMMAND_DISPLAY_IMPORT                  "import"
#define COMMAND_DISPLAY_INFO                    "info"

static void print_usage();
static uint8_t do_cmd_display(int argc, char **argv);
static uint8_t do_cmd_display_import(int argc, char **argv);
static uint8_t do_cmd_display_info(int argc, char **argv);
uint8_t crush_cmd_display_init()
{
        struct crush_container *cmd_display = crush_command_container_define(
                                crush_command_root(), COMMAND_DISPLAY, do_cmd_display);
        crush_command_define(cmd_display, COMMAND_DISPLAY_IMPORT, do_cmd_display_import);
        crush_command_define(cmd_display, COMMAND_DISPLAY_INFO, do_cmd_display_info);

        return CODE_OK;
}
static void print_usage()
{
        printf(
                "Usage:\n"
                "crush display info <display_id> \n"
                "crush display import <import_path> \n"
        );
}
static uint8_t do_cmd_display(int argc, char **argv)
{
        print_usage();
        return CODE_OK;
}
static uint8_t do_cmd_display_import(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_display_info(int argc, char **argv)
{
        return CODE_OK;
}
