#include <crush.h>
#define COMMAND_CONTEXT_NAME                    "context"
#define COMMAND_CONTEXT_DESCRIPTION             "command used to interact with and change the location that crush stores context data"
#define COMMAND_CONTEXT_NEW_NAME                "new"
#define COMMAND_CONTEXT_NEW_DESCRIPTION         "command used to set up a crush user database at a given file-system location"
#define COMMAND_CONTEXT_SET_NAME                "set"
#define COMMAND_CONTEXT_SET_DESCRIPTION         "command used to set the active crush application context to a given location"
#define COMMAND_CONTEXT_PUSH_NAME               "push"
#define COMMAND_CONTEXT_PUSH_DESCRIPTION        "command used to push a new context onto the crush context stack"
#define COMMAND_CONTEXT_POP_NAME                "pop"
#define COMMAND_CONTEXT_POP_DESCRIPTION         "command used to pop one entry off the crush context stack"

static struct light_cli_invocation_result do_cmd_context(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_context_new(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_context_set(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_context_push(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_context_pop(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_context, &cmd_crush, COMMAND_CONTEXT_NAME, COMMAND_CONTEXT_DESCRIPTION, do_cmd_context, 0, 0);
Light_Command_Define(cmd_crush_context_new, &cmd_crush_context, COMMAND_CONTEXT_NEW_NAME, COMMAND_CONTEXT_NEW_DESCRIPTION, do_cmd_context_new, 0, 1);
Light_Command_Define(cmd_crush_context_set, &cmd_crush_context, COMMAND_CONTEXT_SET_NAME, COMMAND_CONTEXT_SET_DESCRIPTION, do_cmd_context_set, 1, 1);
Light_Command_Define(cmd_crush_context_push, &cmd_crush_context, COMMAND_CONTEXT_PUSH_NAME, COMMAND_CONTEXT_PUSH_DESCRIPTION, do_cmd_context_push, 0, 1);
Light_Command_Define(cmd_crush_context_pop, &cmd_crush_context, COMMAND_CONTEXT_POP_NAME, COMMAND_CONTEXT_POP_DESCRIPTION, do_cmd_context_pop, 0, 0);

static void print_usage_context();
static void print_usage_context_new();
static void print_usage_context_set();
static void print_usage_context_push();
static void print_usage_context_pop();
uint8_t crush_cmd_context_init(struct light_command *cmd_parent)
{
        return CODE_OK;
}
struct light_command *crush_cmd_context_get_command()
{
        return &cmd_crush_context;
}
struct light_command *crush_cmd_context_get_subcommand_new()
{
        return &cmd_crush_context_new;
}
struct light_command *crush_cmd_context_get_subcommand_set()
{
        return &cmd_crush_context_set;
}
struct light_command *crush_cmd_context_get_subcommand_push()
{
        return &cmd_crush_context_push;
}
struct light_command *crush_cmd_context_get_subcommand_pop()
{
        return &cmd_crush_context_pop;
}
// shows information about the currently selected CRUSH_CONTEXT, if any
static struct light_cli_invocation_result do_cmd_context(struct light_cli_invocation *invoke)
{
        // pull value of CRUSH_CONTEXT environment variable
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_context_new(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_context_set(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_context_push(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_context_pop(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush context \n"
                "crush context new [<ctx_path>] \n"
                "crush context set [<ctx_path>] \n"
        );
}

static void print_usage_context_new()
{
        printf(
                "Usage:\n"
                "crush context new [<ctx_path>] \n"
        );
}
static void print_usage_context_set()
{
        printf(
                "Usage:\n"
                "crush context set [<ctx_path>] [options] \n"
        );
}
static void print_usage_context_push()
{
        printf(
                "Usage:\n"
                "crush context push [<ctx_path>] [options] \n"
        );
}
static void print_usage_context_pop()
{
        printf(
                "Usage:\n"
                "crush context pop [options] \n"
        );
}