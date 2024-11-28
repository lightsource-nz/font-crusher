#include <crush.h>

#include <light_cli.h>

#define COMMAND_DISPLAY_NAME                    "display"
#define COMMAND_DISPLAY_DESCRIPTION             "command used to interact with entries in the crush display database"
#define COMMAND_DISPLAY_IMPORT_NAME             "import"
#define COMMAND_DISPLAY_IMPORT_DESCRIPTION      "command used to add new display device entries to the local database"
#define COMMAND_DISPLAY_INFO_NAME               "info"
#define COMMAND_DISPLAY_INFO_DESCRIPTION        "command used to show information about a specific display device entry"

static struct light_command *command_crush_display;
static struct light_command *command_crush_display_import;
static struct light_command *command_crush_display_info;

static void print_usage_context();
static void do_cmd_display(struct light_command *command);
static void do_cmd_display_import(struct light_command *command);
static void do_cmd_display_info(struct light_command *command);
uint8_t crush_display_init(struct light_command *cmd_parent)
{
        command_crush_display = light_cli_register_subcommand(
                                                        cmd_parent, COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display);
        command_crush_display_import = light_cli_register_subcommand(command_crush_display, COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import);
        command_crush_display_info = light_cli_register_subcommand(command_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info);

        return CODE_OK;
}
static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush display info <display_id> \n"
                "crush display import <import_path> \n"
        );
}
static void do_cmd_display(struct light_command *command)
{
        print_usage_context();
}
static void do_cmd_display_import(struct light_command *command)
{
        
}
static void do_cmd_display_info(struct light_command *command)
{
        
}
