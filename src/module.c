#include <crush.h>
#include "crush_private.h"

#define COMMAND_MODULE                          "module"
#define COMMAND_MODULE_LIST                     "list"
#define COMMAND_MODULE_INFO                     "info"

static void print_usage();
static uint8_t do_cmd_module(int argc, char **argv);
static uint8_t do_cmd_module_import(int argc, char **argv);
static uint8_t do_cmd_module_info(int argc, char **argv);
uint8_t crush_cmd_module_init()
{
        struct crush_container *cmd_module = crush_command_container_define(
                                crush_command_root(), COMMAND_MODULE, do_cmd_module);
        crush_command_define(cmd_module, COMMAND_MODULE_LIST, do_cmd_module_import);
        crush_command_define(cmd_module, COMMAND_MODULE_INFO, do_cmd_module_info);

        return CODE_OK;
}
static uint8_t do_cmd_module(int argc, char **argv)
{
        print_usage();
        return CODE_OK;
}
static uint8_t do_cmd_module_import(int argc, char **argv)
{
        return CODE_OK;
}
static uint8_t do_cmd_module_info(int argc, char **argv)
{
        return CODE_OK;
}

static void print_usage()
{
        printf(
                "Usage:\n"
                "crush module list \n"
                "crush module info <module_name>\n"
        );
}
