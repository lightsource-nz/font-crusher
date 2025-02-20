#include <crush.h>

#include <jansson.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define COMMAND_MODULE_NAME                    "module"
#define COMMAND_MODULE_DESCRIPTION             "command used to load and unload modules defining extended functionality for crush"
#define COMMAND_MODULE_INFO_NAME               "info"
#define COMMAND_MODULE_INFO_DESCRIPTION        "command used to show info for the named crush module"
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
static struct light_cli_invocation_result do_cmd_module_info(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_list(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_load(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_unload(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_add(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_module_remove(struct light_cli_invocation *command);

Light_Command_Define(cmd_crush_module, &cmd_crush, COMMAND_MODULE_NAME, COMMAND_MODULE_DESCRIPTION, do_cmd_module, 0, 0);
Light_Command_Define(cmd_crush_module_info, &cmd_crush_module, COMMAND_MODULE_INFO_NAME, COMMAND_MODULE_INFO_DESCRIPTION, do_cmd_module_info, 1, 1);
Light_Command_Define(cmd_crush_module_list, &cmd_crush_module, COMMAND_MODULE_LIST_NAME, COMMAND_MODULE_LIST_DESCRIPTION, do_cmd_module_list, 0, 0);
Light_Command_Define(cmd_crush_module_load, &cmd_crush_module, COMMAND_MODULE_LOAD_NAME, COMMAND_MODULE_LOAD_DESCRIPTION, do_cmd_module_load, 1, 2);
Light_Command_Define(cmd_crush_module_unload, &cmd_crush_module, COMMAND_MODULE_UNLOAD_NAME, COMMAND_MODULE_UNLOAD_DESCRIPTION, do_cmd_module_unload, 1, 1);
Light_Command_Define(cmd_crush_module_add, &cmd_crush_module, COMMAND_MODULE_ADD_NAME, COMMAND_MODULE_ADD_DESCRIPTION, do_cmd_module_add, 1, 1);
Light_Command_Define(cmd_crush_module_remove, &cmd_crush_module, COMMAND_MODULE_REMOVE_NAME, COMMAND_MODULE_REMOVE_DESCRIPTION, do_cmd_module_remove, 1, 1);

static void print_usage_module();
static void print_usage_module_info();
static void print_usage_module_list();
static void print_usage_module_load();
static void print_usage_module_unload();
static void print_usage_module_add();
static void print_usage_module_remove();

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:o}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:[]}"

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_MODULE_CONTEXT_OBJECT_NAME

uint8_t crush_module_init()
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
        crush_json_t *obj_data = json_object_get(context->data, id);
        struct crush_module *result = crush_module_object_deserialize(obj_data);
        json_decref(obj_data);
        return result;
}
uint8_t crush_module_context_save(struct crush_module_context *context, const uint8_t *id, struct crush_module *module)
{
        return json_object_set_new(context->data, id, json_pack("{s:o}", id, crush_module_object_serialize(module)));
}
uint8_t crush_module_context_commit(struct crush_module_context *context)
{
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT,
                                        "version",      context->version,
                                        "type",         CRUSH_FONT_CONTEXT_OBJECT_NAME,
                                        "contextFonts", context->data);
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);
        json_decref(obj_data);

        return 0;
}
crush_json_t *crush_module_object_serialize(struct crush_module *module)
{
        json_t *deps = json_array();
        for(uint8_t i = 0; i < module->deps_count; i++) {
                json_array_append_new(deps, json_string(module->deps[i]));
        }
        json_t *files = json_array();
        for(uint8_t i = 0; i < module->file_count; i++) {
                json_array_append_new(files, json_string(module->file[i]));
        }
        json_t *obj = json_pack(
                "{"
                        "s:s,"          //      "name":                 "crush.core"
                        "s:s,"           //      "source_url":           "git:https//github.com/org/lightsource-nz/"
                        "s:s,"          //      "version_string":       "0.1.0"
                        "s:s,"          //      "parent":               "null"
                        "s:s,"           //      "path":                 "modules/crush.core"
                        "s:O,"           //      "deps":
                        "s:O"           //      "files":
                "}",
                "name",         module->name,
                "source_url",   module->source_url,
                "version_str",  module->version_str,
                "parent",       module->parent->name,
                "path",         module->path,
                "deps",         &deps,
                "files",        &files
                );
        return obj;
}
struct crush_module *crush_module_object_deserialize(crush_json_t *data)
{
        struct crush_module *module = light_alloc(sizeof(struct crush_font));
        crush_json_t *files_data, *deps_data;
        json_unpack(data, 
                "{"
                        "s:s,"          //      "name":                 "font_creator.sans_helvetica"
                        "s:s,"          //      "source_url"            "git:https://github.com/font_creator/sans_helvetica"
                        "s:s,"          //      "version_str"           "crush:font:opentype"
                        "s:s,"          //      "parent"                "[git commit hash]"
                        "s:s,"          //      "path"                  "font/font_creator.sans_helvetica"
                        "s:O,"          //      "deps":
                        "s:O"           //      "files":
                "}",
                "name",         &module->name,
                "source_url",   &module->source_url,
                "version_str",  &module->version_str,
                "parent",       &module->parent,
                "path",         &module->path,
                "deps",         &deps_data,
                "files",        &files_data
        );
        uint8_t i;
        json_t *value;
        json_array_foreach(deps_data, i, value) {
                json_unpack(value, "s", &module->deps[i]);
        }
        json_array_foreach(files_data, i, value) {
                json_unpack(value, "s", &module->file[i]);
        }
        json_decref(data);
        json_decref(deps_data);
        json_decref(files_data);

        return module;
}

// shows information about the currently selected CRUSH_MODULE, if any
static struct light_cli_invocation_result do_cmd_module(struct light_cli_invocation *command)
{
        // pull value of CRUSH_MODULE environment variable
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_info(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_list(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_load(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_unload(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_add(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_module_remove(struct light_cli_invocation *command)
{
        return Result_Success;
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
static void print_usage_module_info()
{
        printf(
                "Usage:\n"
                "crush module info <module_name> [options] \n"
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