#include <crush.h>
#include "crush_private.h"

#define COMMAND_FONT                            "font"
#define COMMAND_FONT_IMPORT                     "import"
#define COMMAND_FONT_INFO                       "info"

static void print_usage();
static uint8_t do_cmd_font(int argc, char **argv);
static uint8_t do_cmd_font_import(int argc, char **argv);
static uint8_t do_cmd_font_info(int argc, char **argv);
uint8_t crush_cmd_font_init()
{
        struct crush_container *cmd_font = crush_command_container_define(
                                crush_command_root(), COMMAND_FONT, do_cmd_font);
        crush_command_define(cmd_font, COMMAND_FONT_IMPORT, do_cmd_font_import);
        crush_command_define(cmd_font, COMMAND_FONT_INFO, do_cmd_font_info);

        return CODE_OK;
}
static uint8_t do_cmd_font(int argc, char **argv)
{
        print_usage();
        return CODE_OK;
}
static uint8_t do_cmd_font_import(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_font_info(int argc, char **argv)
{
        return CODE_OK;
}

static void print_usage()
{
        printf(
                "Usage:\n"
                "crush font info <font_id> \n"
                "crush font add <font_id> <file_path> \n"
        );
}
