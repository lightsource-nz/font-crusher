#include <crush.h>

#include <light_cli.h>

#define COMMAND_DISPLAY_NAME                    "display"
#define COMMAND_DISPLAY_DESCRIPTION             "command used to interact with entries in the crush display database"
#define COMMAND_DISPLAY_IMPORT_NAME             "import"
#define COMMAND_DISPLAY_IMPORT_DESCRIPTION      "command used to add new display device entries to the local database"
#define COMMAND_DISPLAY_INFO_NAME               "info"
#define COMMAND_DISPLAY_INFO_DESCRIPTION        "command used to show information about a specific display device entry"
#define COMMAND_DISPLAY_LIST_NAME               "list"
#define COMMAND_DISPLAY_LIST_DESCRIPTION        "command used to list all entries in the display database"

static struct light_command *cmd_display;
static struct light_command *cmd_display_import;
static struct light_command *cmd_display_info;
static struct light_command *cmd_display_list;

static void print_usage_context();
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command);

Light_Command_Define(cmd_crush_display, &cmd_crush, COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display, 0, 0);
Light_Command_Define(cmd_crush_display_import, &cmd_crush_display, COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import, 1, 1);
Light_Command_Define(cmd_crush_display_info, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);
Light_Command_Define(cmd_crush_display_list, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);

uint8_t crush_display_init(struct light_command *cmd_parent)
{
        /*
        cmd_display = light_cli_register_subcommand(cmd_parent,
                COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display);
        cmd_display_import = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import);
        cmd_display_info = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info);
        */
       
        return CODE_OK;
}
struct light_command *crush_display_get_command()
{
        return cmd_display;
}
struct light_command *crush_display_get_subcommand_import()
{
        return cmd_display_import;
}
struct light_command *crush_display_get_subcommand_info()
{
        return cmd_display_info;
}
static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush display info <display_id> \n"
                "crush display import <import_path> \n"
        );
}
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command)
{
        print_usage_context();
}
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
