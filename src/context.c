#include <crush.h>
#include "crush_private.h"

#define COMMAND_CONTEXT                         "context"
#define COMMAND_CONTEXT_NEW                     "new"
#define COMMAND_CONTEXT_SET                     "set"

static void print_usage();
static uint8_t do_cmd_context_new(int argc, char **argv);
static uint8_t do_cmd_context_set(int argc, char **argv);
uint8_t crush_cmd_context_init()
{
        uint8_t id_context = crush_command_container_define(
                                CRUSH_COMMAND_PARENT_NONE, COMMAND_CONTEXT);
        crush_command_define(id_context, COMMAND_CONTEXT_NEW, do_cmd_context_new);
        crush_command_define(id_context, COMMAND_CONTEXT_SET, do_cmd_context_set);

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
