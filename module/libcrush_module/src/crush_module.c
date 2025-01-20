#include <crush.h>

#include <jansson.h>
#include <sys/stat.h>
#include <errno.h>

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

static struct light_cli_invocation_result do_cmd_module(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_list(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_load(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_unload(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_add(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_remove(struct light_cli_invocation *command);

Light_Command_Define(cmd_crush_module, &cmd_crush, COMMAND_MODULE_NAME, COMMAND_MODULE_DESCRIPTION, do_cmd_module, 0, 0);
Light_Command_Define(cmd_crush_module_list, &cmd_crush_module, COMMAND_MODULE_LIST_NAME, COMMAND_MODULE_LIST_DESCRIPTION, do_cmd_module_list, 0, 0);
Light_Command_Define(cmd_crush_module_load, &cmd_crush_module, COMMAND_MODULE_LOAD_NAME, COMMAND_MODULE_LOAD_DESCRIPTION, do_cmd_module_load, 1, 2);
Light_Command_Define(cmd_crush_module_unload, &cmd_crush_module, COMMAND_MODULE_UNLOAD_NAME, COMMAND_MODULE_UNLOAD_DESCRIPTION, do_cmd_module_unload, 1, 1);
Light_Command_Define(cmd_crush_module_add, &cmd_crush_module, COMMAND_MODULE_ADD_NAME, COMMAND_MODULE_ADD_DESCRIPTION, do_cmd_module_add, 1, 1);
Light_Command_Define(cmd_crush_module_remove, &cmd_crush_module, COMMAND_MODULE_REMOVE_NAME, COMMAND_MODULE_REMOVE_DESCRIPTION, do_cmd_module_remove, 1, 1);

static void print_usage_module();
static void print_usage_module_list();
static void print_usage_module_load();
static void print_usage_module_unload();
static void print_usage_module_add();
static void print_usage_module_remove();

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_MODULE_CONTEXT_OBJECT_NAME

uint8_t crush_module_init(struct light_command *cmd_parent)
{
        crush_common_register_context_object_loader(CRUSH_MODULE_CONTEXT_OBJECT_NAME, CRUSH_MODULE_CONTEXT_JSON_FILE,
                                        crush_font_create_context, crush_font_load_context);
        return CODE_OK;
}
crush_json_t *crush_module_create_context()
{
        json_t *module_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:module",
                        "s:["                   // "contextModules"
                                "{"
                                        "s:s,"          //      "name":                 "crush.core"
                                        "s:i,"          //      "version_seq"           "0"
                                        "s:s,"          //      "version_string"        "0.1.0"
                                        "s:s"           //      "mod_root"              "modules/crush.core"
                                        "s:[]"          //      "dependencies"
                                "}"
                        "]"
                "}",
                "version", SCHEMA_VERSION, "type", OBJECT_NAME, "contextModules", "name", "crush.core",
                "version_seq", "0", "version_string", "0.1.0", "mod_root", "modules/crush.core", "dependencies");
        return module_obj;
}
void crush_module_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_module_context *mod_ctx = light_alloc(sizeof(struct crush_module_context));
        mod_ctx->root = context;
        mod_ctx->file_path = file_path;
        json_unpack(
                data,
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:module",
                        "s:o"                   // "contextModules"
                "}",
                "version", &mod_ctx->version, "type", &type,
                "contextModules", &mod_ctx->data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, OBJECT_NAME, mod_ctx);
}
struct crush_module *crush_module_context_get(struct crush_module_context *context, const uint8_t *id)
{

}
uint8_t crush_module_context_save(struct crush_module_context *context, const uint8_t *id, struct crush_module *font);
uint8_t crush_module_context_commit(struct crush_module_context *context);
// shows information about the currently selected CRUSH_MODULE, if any
static struct light_cli_invocation_result do_cmd_module(struct light_cli_invocation *command)
{
        // pull value of CRUSH_MODULE environment variable
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_module_list(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_module_load(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_module_unload(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_module_add(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_module_remove(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
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