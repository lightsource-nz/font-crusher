#include <crush.h>
#include "crush_private.h"

#define COMMAND_FONT                            "font"
#define COMMAND_FONT_IMPORT                     "import"
#define COMMAND_FONT_INFO                       "info"

static void print_usage();
static uint8_t do_cmd_font_import(int argc, char **argv);
static uint8_t do_cmd_font_info(int argc, char **argv);
uint8_t crush_cmd_font_init()
{
        uint8_t id_font = crush_command_container_define(
                                CRUSH_COMMAND_PARENT_NONE, COMMAND_FONT);
        crush_command_define(id_font, COMMAND_FONT_IMPORT, do_cmd_font_import);
        crush_command_define(id_font, COMMAND_FONT_INFO, do_cmd_font_info);

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
