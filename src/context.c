#include <crush.h>
#include "crush_private.h"

#define COMMAND_CONTEXT                         "context"
#define COMMAND_CONTEXT_NEW                     "new"
#define COMMAND_CONTEXT_SET                     "set"

static void print_usage();
static uint8_t do_cmd_context(int argc, char **argv);
static uint8_t do_cmd_context_new(int argc, char **argv);
static uint8_t do_cmd_context_set(int argc, char **argv);
uint8_t crush_cmd_context_init()
{
        struct crush_container *cmd_context = crush_command_container_define(
                                crush_command_root(), COMMAND_CONTEXT, do_cmd_context);
        crush_command_define(cmd_context, COMMAND_CONTEXT_NEW, do_cmd_context_new);
        crush_command_define(cmd_context, COMMAND_CONTEXT_SET, do_cmd_context_set);

        return CODE_OK;
}
static uint8_t do_cmd_context(int argc, char **argv)
{
        print_usage();
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
static void print_usage()
{
        printf(
                "Usage:\n"
                "crush context \n"
                "crush context new \n"
                "crush context set <ctx_path> \n"
        );
}
