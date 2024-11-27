#include <crush.h>
#include "crush_private.h"

#include <light_cli.h>

#define COMMAND_FONT_NAME                       "font"
#define COMMAND_FONT_DESCRIPTION                "command used to interact with and manage font data"
#define COMMAND_FONT_IMPORT_NAME                "import"
#define COMMAND_FONT_IMPORT_DESCRIPTION         "command used to add new font data to the local font database from a variety of sources"
#define COMMAND_FONT_INFO_NAME                  "info"
#define COMMAND_FONT_INFO_DESCRIPTION           "command used to show information about fonts in the local database"

static void print_usage_context();
static uint8_t do_cmd_font(struct light_command *command);
static uint8_t do_cmd_font_import(struct light_command *command);
static uint8_t do_cmd_font_info(struct light_command *command);
uint8_t crush_cmd_font_init(struct light_command *cmd_parent)
{
        struct light_command *cmd_font = light_cli_register_subcommand(
                                                        cmd_parent, COMMAND_FONT_NAME, COMMAND_FONT_DESCRIPTION, do_cmd_font);
        light_cli_register_subcommand(cmd_font, COMMAND_FONT_IMPORT_NAME, COMMAND_FONT_IMPORT_DESCRIPTION, do_cmd_font_import);
        light_cli_register_subcommand(cmd_font, COMMAND_FONT_INFO_NAME, COMMAND_FONT_INFO_DESCRIPTION, do_cmd_font_info);

        return CODE_OK;
}
static uint8_t do_cmd_font(struct light_command *command)
{
        print_usage_context();
        return CODE_OK;
}
static uint8_t do_cmd_font_import(struct light_command *command)
{
        return CODE_OK;
}
static uint8_t do_cmd_font_info(struct light_command *command)
{
        return CODE_OK;
}

static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush font info <font_id> \n"
                "crush font add <font_id> <file_path> \n"
        );
}
