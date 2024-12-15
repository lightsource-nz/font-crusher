#include <crush.h>

#define COMMAND_RENDER_NAME                     "render"
#define COMMAND_RENDER_DESCRIPTION              "command used to access text rendering functions, and the data produced by them"
#define COMMAND_RENDER_NEW_NAME                 "new"
#define COMMAND_RENDER_NEW_DESCRIPTION          "command used to render a new set of character glyphs from a font file and screen information"
#define COMMAND_RENDER_INFO_NAME                "info"
#define COMMAND_RENDER_INFO_DESCRIPTION         "command used to view information about a given render data object"
#define COMMAND_RENDER_LIST_NAME                "list"
#define COMMAND_RENDER_LIST_DESCRIPTION         "command used to view information about a given render data object"

static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_render, &cmd_crush, COMMAND_RENDER_NAME, COMMAND_RENDER_DESCRIPTION, do_cmd_render, 0, 0);
Light_Command_Define(cmd_crush_render_new, &cmd_crush_render, COMMAND_RENDER_NEW_NAME, COMMAND_RENDER_NEW_DESCRIPTION, do_cmd_render_new, 2, 3);
Light_Command_Define(cmd_crush_render_info, &cmd_crush_render, COMMAND_RENDER_INFO_NAME, COMMAND_RENDER_INFO_DESCRIPTION, do_cmd_render_info, 0, 1);
Light_Command_Define(cmd_crush_render_list, &cmd_crush_render, COMMAND_RENDER_LIST_NAME, COMMAND_RENDER_LIST_DESCRIPTION, do_cmd_render_list, 1, 2);

static void print_usage_context();
static void print_usage_context_new();
static void print_usage_context_set();
uint8_t crush_render_init(struct light_command *cmd_parent)
{
        /*
        cmd_render = light_cli_register_subcommand(cmd_parent,
                COMMAND_RENDER_NAME, COMMAND_RENDER_DESCRIPTION, do_cmd_render);
        cmd_render_new = light_cli_register_subcommand(cmd_render,
                COMMAND_RENDER_NEW_NAME, COMMAND_RENDER_NEW_DESCRIPTION, do_cmd_render_new);
        cmd_render_info = light_cli_register_subcommand(cmd_render,
                COMMAND_RENDER_INFO_NAME, COMMAND_RENDER_INFO_DESCRIPTION, do_cmd_render_info);
        */
        return CODE_OK;
}
static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke)
{
        print_usage_context();
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}

static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush render list \n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
