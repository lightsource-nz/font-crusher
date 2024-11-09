#include <crush.h>
#include "crush_private.h"

#define COMMAND_CONTEXT                         "context"
#define COMMAND_CONTEXT_NEW                     "new"
#define COMMAND_CONTEXT_SET                     "set"

static void print_usage_context();
static void print_usage_context_new();
static void print_usage_context_set();
static uint8_t do_cmd_context(int argc, char **argv);
static uint8_t do_cmd_context_new(int argc, char **argv);
static uint8_t do_cmd_context_set(int argc, char **argv);
uint8_t crush_cmd_context_init(struct crush_container *cmd_parent)
{
        struct crush_container *cmd_context = crush_command_container_define(
                                                        cmd_parent, COMMAND_CONTEXT, do_cmd_context);
        crush_command_define(cmd_context, COMMAND_CONTEXT_NEW, do_cmd_context_new);
        crush_command_define(cmd_context, COMMAND_CONTEXT_SET, do_cmd_context_set);

        return CODE_OK;
}
// shows information about the currently selected CRUSH_CONTEXT, if any
static uint8_t do_cmd_context(int argc, char **argv)
{
        // pull value of CRUSH_CONTEXT environment variable
        
        return CODE_OK;
}
static uint8_t do_cmd_context_new(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_context_set(int argc, char **argv)
{
        return CODE_OK;
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
                "crush context set [<ctx_path>] \n"
        );
}