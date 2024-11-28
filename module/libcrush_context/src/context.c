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

static struct light_command *cmd_context;
static struct light_command *cmd_context_new;
static struct light_command *cmd_context_set;
static struct light_command *cmd_context_push;
static struct light_command *cmd_context_pop;

static void print_usage_context();
static void print_usage_context_new();
static void print_usage_context_set();
static void print_usage_context_push();
static void print_usage_context_pop();
static void do_cmd_context(struct light_command *command);
static void do_cmd_context_new(struct light_command *command);
static void do_cmd_context_set(struct light_command *command);
static void do_cmd_context_push(struct light_command *command);
static void do_cmd_context_pop(struct light_command *command);
uint8_t crush_context_init(struct light_command *cmd_parent)
{
        cmd_context = light_cli_register_subcommand(cmd_parent,
                COMMAND_CONTEXT_NAME, COMMAND_CONTEXT_DESCRIPTION, do_cmd_context);
        cmd_context_new = light_cli_register_subcommand(cmd_context,
                COMMAND_CONTEXT_NEW_NAME, COMMAND_CONTEXT_NEW_DESCRIPTION, do_cmd_context_new);
        cmd_context_set = light_cli_register_subcommand(cmd_context,
                COMMAND_CONTEXT_SET_NAME, COMMAND_CONTEXT_SET_DESCRIPTION, do_cmd_context_set);
        cmd_context_push = light_cli_register_subcommand(cmd_context,
                COMMAND_CONTEXT_PUSH_NAME, COMMAND_CONTEXT_PUSH_DESCRIPTION, do_cmd_context_push);
        cmd_context_pop = light_cli_register_subcommand(cmd_context,
                COMMAND_CONTEXT_POP_NAME, COMMAND_CONTEXT_POP_DESCRIPTION, do_cmd_context_pop);

        return CODE_OK;
}
struct light_command *crush_context_get_command()
{
        return cmd_context;
}
struct light_command *crush_context_get_subcommand_new()
{
        return cmd_context_new;
}
struct light_command *crush_context_get_subcommand_set()
{
        return cmd_context_set;
}
struct light_command *crush_context_get_subcommand_push()
{
        return cmd_context_push;
}
struct light_command *crush_context_get_subcommand_pop()
{
        return cmd_context_pop;
}
// shows information about the currently selected CRUSH_CONTEXT, if any
static void do_cmd_context(struct light_command *command)
{
        // pull value of CRUSH_CONTEXT environment variable
        
}
static void do_cmd_context_new(struct light_command *command)
{
        
}
static void do_cmd_context_set(struct light_command *command)
{
        
}
static void do_cmd_context_push(struct light_command *command)
{
        
}
static void do_cmd_context_pop(struct light_command *command)
{
        
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