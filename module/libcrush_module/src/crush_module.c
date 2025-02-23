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

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:i,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:i,s:[]}"

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_MODULE_CONTEXT_OBJECT_NAME

const uint8_t *crush_module_get_name(struct crush_module *module)
{
        return module->name;
}
const uint8_t *crush_module_get_version_str(struct crush_module *module)
{
        return module->version_str;
}
struct crush_module *crush_module_get_parent(struct crush_module *module)
{
        return module->parent;
}
const uint8_t *crush_module_get_source_url(struct crush_module *module)
{
        return module->source_url;
}
const uint8_t *crush_module_get_path(struct crush_module *module)
{
        return module->path;
}
uint8_t crush_module_get_deps_count(struct crush_module *module)
{
        return module->deps_count;
}
struct crush_module *crush_module_get_deps(struct crush_module *module, uint8_t index)
{
        if(index >= module->deps_count)
                return NULL;
        return module->deps[index];
}
uint8_t crush_module_get_file_count(struct crush_module *module)
{
        return module->file_count;
}
uint8_t *crush_module_get_file(struct crush_module *module, uint8_t index)
{
        if(index >= module->file_count)
                return NULL;
        return module->file[index];
}
bool crush_module_has_direct_dependency(struct crush_module *module, struct crush_module *other)
{
        for(uint8_t i = 0; i < other->deps_count; i++) {
                if(strcmp(crush_module_get_name(other->deps[i]), crush_module_get_name(module))) {
                        return true;
                }
        }
        return false;
}
#define DEPENDENCY_MAX_DEPTH    16
static bool depends_on_recurse(struct crush_module *module, struct crush_module *other, uint8_t depth)
{
        if(depth == 0) {
                return false;
        }
        for(uint8_t i = 0; i < other->deps_count; i++) {
                if(strcmp(crush_module_get_name(other->deps[i]), crush_module_get_name(module))) {
                        return true;
                }
                if(depends_on_recurse(module, other->deps[i], depth - 1)) {
                        return true;
                }
        }
        return false;
}
bool crush_module_depends_on(struct crush_module *module, struct crush_module *other)
{
        return depends_on_recurse(module, other, DEPENDENCY_MAX_DEPTH);
}

void crush_module_init(struct crush_module *module, struct crush_module_context *context, struct crush_module *parent, const uint8_t *name, const uint16_t version_seq, const uint8_t *source_url, const uint8_t *path)
{
        module->context = context;
        module->parent = parent;
        module->name = name;
        module->source_url = source_url;
        module->version_seq = version_seq;
        module->path = path;
        module->state = CRUSH_MODULE_STATE_NEW;
        module->deps_count = 0;
        module->file_count = 0;
}
void crush_module_release(struct crush_module *module)
{
        light_free(module);
}
// list accessors return LIGHT_NO_RESOURCE on list overflow
uint8_t crush_module_add_dependency(struct crush_module *module, struct crush_module *dependency)
{
        if(module->deps_count >= CRUSH_MODULE_DEPS_MAX) {
                light_error("failed to add dependency '%s' to crush module '%s': max modules reached",
                                crush_module_get_name(dependency), crush_module_get_name(module));
                return LIGHT_NO_RESOURCE;
        }
        module->deps[module->deps_count++] = dependency;
}
uint8_t crush_module_add_file(struct crush_module *module, uint8_t *filename)
{
        if(module->deps_count >= CRUSH_MODULE_FILE_MAX) {
                light_error("failed to add file '%s' to crush module '%s': max files reached",
                                                        filename, crush_module_get_name(module));
                return LIGHT_NO_RESOURCE;
        }
        module->file[module->file_count++] = filename;
}
uint8_t crush_module_onload()
{
        crush_common_register_context_object_loader(CRUSH_MODULE_CONTEXT_OBJECT_NAME, CRUSH_MODULE_CONTEXT_JSON_FILE,
                                        crush_font_create_context, crush_font_load_context);
        return CODE_OK;
}
struct crush_module_context *crush_module_context()
{
        return crush_module_get_context(crush_context());
}
struct crush_module_context *crush_module_get_context(struct crush_context *root)
{
        return crush_context_get_context_object_type(root, OBJECT_NAME, struct crush_module_context *);
}
crush_json_t *crush_module_create_context()
{
        uint32_t next_id = crush_common_get_initial_counter_value();
        json_t *module_obj = json_pack(
                CONTEXT_OBJECT_NEW_FMT,
                "version", SCHEMA_VERSION,
                "type", OBJECT_NAME,
                "next_id", next_id,
                "contextModules");
        return module_obj;
}
void crush_module_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_module_context *mod_ctx = light_alloc(sizeof(struct crush_module_context));
        mod_ctx->root = context;
        mod_ctx->file_path = file_path;
        json_unpack(data, CONTEXT_OBJECT_FMT, "version", &mod_ctx->version, "type", &type,"next_id", &mod_ctx->next_id,
                "contextModules", &mod_ctx->data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, OBJECT_NAME, mod_ctx);
}
struct crush_module *crush_module_context_get(struct crush_module_context *context, const uint32_t id)
{
        ID_To_String(id_str, id);
        crush_json_t *obj_data = json_object_getn(context->data, id_str, CRUSH_JSON_KEY_LENGTH);
        struct crush_module *result = crush_module_object_deserialize(obj_data);
        json_decref(obj_data);
        result->context = context;
        return result;
}
struct crush_module *crush_module_context_get_by_name(struct crush_module_context *context, const uint8_t *name)
{
        const uint8_t *_key;
        json_t *_val;
        // TODO place sync barriers around access to the object store
        json_object_foreach(context->data, _key, _val) {
                if(strcmp(json_string_value(json_object_get(_val, "name")), name)) {
                        struct crush_module *out = crush_module_object_deserialize(_val);
                        json_decref(_val);
                        return out;
                }
                json_decref(_val);
        }
}
uint8_t crush_module_context_save(struct crush_module_context *context, struct crush_module *object)
{
        // if crush_module_save() is called on an object with no context attached, attach
        // object to the current context
        if(!context && !object->context)
                context = object->context = crush_module_context();
        if(!context)
                context = object->context;
        if(!object->context)
                object->context = context;
        if(object->id == CRUSH_JSON_ID_NEW) {
                light_debug("saving new object, name: '%s'", object->name);
                uint32_t id_old, id_new;
                do {
                        id_old = context->next_id;
                        id_new = crush_common_get_next_counter_value(id_old);
                } while(!atomic_compare_exchange_weak(&context->next_id, &id_old, id_new));
                object->id = id_old;
        } else {
                light_debug("saving object ID 0x%8X, name: '%s'", object->id, object->name);
        }
        ID_To_String(id_str, object->id);
        return json_object_setn_new(context->data, id_str, CRUSH_JSON_KEY_LENGTH, crush_module_object_serialize(object));
}
uint8_t crush_module_context_commit(struct crush_module_context *context)
{
        light_debug("writing context '%s' to disk", context->file_path);
        ID_To_String(id_str, context->next_id);
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT,
                                        "version",      context->version,
                                        "type",         CRUSH_MODULE_CONTEXT_OBJECT_NAME,
                                        "contextModules", context->data);
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
                json_array_append_new(deps, json_string(crush_module_get_name(module->deps[i])));
        }
        json_t *files = json_array();
        for(uint8_t i = 0; i < module->file_count; i++) {
                json_array_append_new(files, json_string(module->file[i]));
        }
        json_t *obj = json_pack(
                "{"
                        "s:s,"          //      "name":                 "crush.core"
                        "s:s,"          //      "source_url":           "git:https//github.com/org/lightsource-nz/"
                        "s:s,"          //      "version_string":       "0.1.0"
                        "s:s,"          //      "parent":               "null"
                        "s:s,"          //      "path":                 "modules/crush.core"
                        "s:O,"          //      "deps":
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