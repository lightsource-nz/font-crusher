#include <crush.h>
#include "crush_private.h"

#define COMMAND_DISPLAY                         "display"
#define COMMAND_DISPLAY_IMPORT                  "import"
#define COMMAND_DISPLAY_INFO                    "info"

static void print_usage();
static uint8_t do_cmd_display_import(int argc, char **argv);
static uint8_t do_cmd_display_info(int argc, char **argv);
uint8_t crush_cmd_display_init()
{
        uint8_t id_display = crush_command_container_define(
                                CRUSH_COMMAND_PARENT_NONE, COMMAND_DISPLAY);
        crush_command_define(id_display, COMMAND_DISPLAY_IMPORT, do_cmd_display_import);
        crush_command_define(id_display, COMMAND_DISPLAY_INFO, do_cmd_display_info);

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
static uint8_t do_cmd_display_import(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_display_info(int argc, char **argv)
{
        return CODE_OK;
}
