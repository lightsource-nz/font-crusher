#include <crush.h>

#define COMMAND_MODULE_NAME                    "module"
#define COMMAND_MODULE_DESCRIPTION             "command used to load and unload modules defining extended functionality for crush"
#define COMMAND_MODULE_LIST_NAME               "list"
#define COMMAND_MODULE_LIST_DESCRIPTION        "command used to list all modules currently available in this crush context"
#define COMMAND_MODULE_LOAD_NAME               "load"
#define COMMAND_MODULE_LOAD_DESCRIPTION        "command used to load an available crush module and activate it in this crush context"
#define COMMAND_MODULE_UNLOAD_NAME             "unload"
#define COMMAND_MODULE_UNLOAD_DESCRIPTION      "command used to deactivate an active crush module in this crush context"
#define COMMAND_MODULE_ADD_NAME                "add"
#define COMMAND_MODULE_ADD_DESCRIPTION         "command used to add a new crush module from a file or URL and make it available in this crush context"
#define COMMAND_MODULE_REMOVE_NAME             "remove"
#define COMMAND_MODULE_REMOVE_DESCRIPTION      "command used to remove an available crush module from this crush context"

static struct light_command *command_crush_module;
static struct light_command *command_crush_module_list;
static struct light_command *command_crush_module_load;
static struct light_command *command_crush_module_unload;
static struct light_command *command_crush_module_add;
static struct light_command *command_crush_module_remove;

static void print_usage_module();
static void print_usage_module_list();
static void print_usage_module_load();
static void print_usage_module_unload();
static void print_usage_module_add();
static void print_usage_module_remove();
static void do_cmd_module(struct light_command *command);
static void do_cmd_module_list(struct light_command *command);
static void do_cmd_module_load(struct light_command *command);
static void do_cmd_module_unload(struct light_command *command);
static void do_cmd_module_add(struct light_command *command);
static void do_cmd_module_remove(struct light_command *command);
uint8_t crush_module_init(struct light_command *cmd_parent)
{
        command_crush_module = light_cli_register_subcommand(
                                                        cmd_parent, COMMAND_MODULE_NAME, COMMAND_MODULE_DESCRIPTION, do_cmd_module);
        command_crush_module_list = light_cli_register_subcommand(command_crush_module, COMMAND_MODULE_LIST_NAME, COMMAND_MODULE_LIST_DESCRIPTION, do_cmd_module_list);
        command_crush_module_load = light_cli_register_subcommand(command_crush_module, COMMAND_MODULE_LOAD_NAME, COMMAND_MODULE_LOAD_DESCRIPTION, do_cmd_module_load);
        command_crush_module_unload = light_cli_register_subcommand(command_crush_module, COMMAND_MODULE_UNLOAD_NAME, COMMAND_MODULE_UNLOAD_DESCRIPTION, do_cmd_module_unload);
        command_crush_module_add = light_cli_register_subcommand(command_crush_module, COMMAND_MODULE_ADD_NAME, COMMAND_MODULE_ADD_DESCRIPTION, do_cmd_module_add);
        command_crush_module_remove = light_cli_register_subcommand(command_crush_module, COMMAND_MODULE_REMOVE_NAME, COMMAND_MODULE_REMOVE_DESCRIPTION, do_cmd_module_remove);

        return CODE_OK;
}
// shows information about the currently selected CRUSH_MODULE, if any
static void do_cmd_module(struct light_command *command)
{
        // pull value of CRUSH_MODULE environment variable
        
}
static void do_cmd_module_list(struct light_command *command)
{
        
}
static void do_cmd_module_load(struct light_command *command)
{
        
}
static void do_cmd_module_unload(struct light_command *command)
{
        
}
static void do_cmd_module_add(struct light_command *command)
{

}
static void do_cmd_module_remove(struct light_command *command)
{
        
}
static void print_usage_module()
{
        printf(
                "Usage:\n"
                "crush module\n"
                "crush module list [-f <filter_expr>]\n"
                "crush module load <name>\n"
                "crush module add <url>\n"
        );
}

static void print_usage_module_list()
{
        printf(
                "Usage:\n"
                "crush module list [-f <filter_expr>] \n"
        );
}
static void print_usage_module_load()
{
        printf(
                "Usage:\n"
                "crush module load <module> \n"
        );
}
static void print_usage_module_unload()
{
        printf(
                "Usage:\n"
                "crush module unload <module> \n"
        );
}
static void print_usage_module_add()
{
        printf(
                "Usage:\n"
                "crush module add <module_path> \n"
        );
}
static void print_usage_module_remove()
{
        printf(
                "Usage:\n"
                "crush module remove <module> \n"
        );
}