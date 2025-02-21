#include <crush.h>

#include <jansson.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define COMMAND_FONT_NAME "font"
#define COMMAND_FONT_DESCRIPTION "main subcommand for access to the crush font database (aliased to 'font list')"
#define COMMAND_FONT_ADD_NAME "add"
#define COMMAND_FONT_ADD_DESCRIPTION "adds a font to the local crush database, from a web URL or local file"
#define COMMAND_FONT_REMOVE_NAME "remove"
#define COMMAND_FONT_REMOVE_DESCRIPTION "removes a font from the local crush database"
#define COMMAND_FONT_INFO_NAME "info"
#define COMMAND_FONT_INFO_DESCRIPTION "displays information about a font in the local crush database"
#define COMMAND_FONT_LIST_NAME "list"
#define COMMAND_FONT_LIST_DESCRIPTION "displays a list of all fonts stored in the local crush database"

static void print_usage_font();
static void print_usage_font_add();
static void print_usage_font_remove();
static void print_usage_font_info();
static void print_usage_font_list();
static struct light_cli_invocation_result do_cmd_font(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_add(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_remove(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_info(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_list(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_font, &cmd_crush, COMMAND_FONT_NAME, COMMAND_FONT_DESCRIPTION, do_cmd_font, 0, 0);
Light_Command_Define(cmd_crush_font_add, &cmd_crush_font, COMMAND_FONT_ADD_NAME, COMMAND_FONT_ADD_DESCRIPTION, do_cmd_font_add, 1, 1);
Light_Command_Define(cmd_crush_font_remove, &cmd_crush_font, COMMAND_FONT_REMOVE_NAME, COMMAND_FONT_REMOVE_DESCRIPTION, do_cmd_font_remove, 1, 1);
Light_Command_Define(cmd_crush_font_info, &cmd_crush_font, COMMAND_FONT_INFO_NAME, COMMAND_FONT_INFO_DESCRIPTION, do_cmd_font_info, 1, 1);
Light_Command_Define(cmd_crush_font_list, &cmd_crush_font, COMMAND_FONT_LIST_NAME, COMMAND_FONT_LIST_DESCRIPTION, do_cmd_font_list, 0, 0);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_FONT_CONTEXT_OBJECT_NAME
#define JSON_FILE CRUSH_FONT_CONTEXT_JSON_FILE

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:o}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:[]}"

uint8_t crush_font_init()
{
        crush_common_register_context_object_loader(OBJECT_NAME, JSON_FILE,
                                        crush_font_create_context, crush_font_load_context);
        return LIGHT_OK;
}
struct crush_font_context *crush_font_context()
{
        return crush_font_get_context(crush_context());
}
struct crush_font_context *crush_font_get_context(struct crush_context *root)
{
        return crush_context_get_context_object_type(root, OBJECT_NAME, struct crush_font_context *);
}
crush_json_t *crush_font_create_context()
{
        return json_pack(CONTEXT_OBJECT_NEW_FMT, "version", SCHEMA_VERSION, "type", OBJECT_NAME, "contextFonts");
}
void crush_font_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_font_context *font_ctx = light_alloc(sizeof(struct crush_font_context));
        font_ctx->root = context;
        font_ctx->file_path = file_path;
        json_unpack(data, CONTEXT_OBJECT_FMT, "version", &font_ctx->version, "type", &type, "contextFonts", &font_ctx->data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, OBJECT_NAME, font_ctx);
}

struct crush_font *crush_font_context_get(struct crush_font_context *context, const uint8_t *id)
{
        crush_json_t *obj_data = json_object_get(context->data, id);
        struct crush_font *result = crush_font_object_deserialize(obj_data);
        json_decref(obj_data);
        return result;
}
struct crush_font *crush_font_context_get_by_name(struct crush_font_context *context, const uint8_t *name)
{
        const char *key;
        json_t *value;
        json_object_foreach(context->data, key, value) {
                if(strcmp(json_string_value(json_object_get(value, "name")), name)) {
                        struct crush_font *out = crush_font_object_deserialize(value);
                        json_decref(value);
                        return out;
                }
                json_decref(value);
        }
        return NULL;
}
uint8_t crush_font_context_save(struct crush_font_context *context, struct crush_font *object)
{
        if(object->id == CRUSH_JSON_ID_NEW) {
                light_debug("saving new object, name: '%s'", object->name);
        } else {
                light_debug("saving object ID 0x%8X, name: '%s'", object->id, object->name);
        }
        ID_To_String(id_str, object->id);
        return json_object_set_new(context->data, id_str, crush_font_object_serialize(object));
}
uint8_t crush_font_context_commit(struct crush_font_context *context)
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

crush_json_t *crush_font_object_serialize(struct crush_font *font)
{
        json_t *files = json_array();
        for(uint8_t i = 0; i < font->file_count; i++) {
                json_array_append_new(files, json_string(font->file[i]));
        }
        json_t *obj = json_pack(
                "{"
                        "s:s,"          //      "name":                 "org.font_name"
                        "s:s"           //      "source_uri":           "git:https//github.com/org/font_name"
                        "s:s,"          //      "version_string":       "0.1.0"
                        "s:s"           //      "mod_root":             "modules/crush.core"
                        "s:O"           //      "files":
                "}",
                "name",         font->name,
                "source_url",   font->source_url,
                "font_type",    font->font_type,
                "files",        &files
                );
        return obj;
}
// ==> struct crush_font *crush_font_object_deserialize(crush_json_t data)
//   we may come back and implement full custom field extraction for objects of differing
// font_type values, but for now all font objects are expected to have the same fields
struct crush_font *crush_font_object_deserialize(crush_json_t *data)
{
        struct crush_font *font = light_alloc(sizeof(struct crush_font));
        crush_json_t *files_data;
        json_unpack(data, 
                "{"
                        "s:s,"          //      "name":                 "font_creator.sans_helvetica"
                        "s:s,"          //      "source_url"            "git:https://github.com/font_creator/sans_helvetica"
                        "s:s,"          //      "font_type"             "crush:font:opentype"
                        "s:s,"          //      "font_version"          "[git commit hash]"
                        "s:s,"          //      "font_root"             "font/font_creator.sans_helvetica"
                        "s:O"           //      "files"
                "}",
                "name",         &font->name,
                "source_url",   &font->source_url,
                "font_type",    &font->font_type,
                "font_version", &font->font_version,
                "font_root",    &font->path,
                "files",        &files_data
        );
        uint8_t i;
        json_t *file_value;
        json_array_foreach(files_data, i, file_value) {
                json_unpack(file_value, "s", &font->file[i]);
        }
        json_decref(data);

        return font;
}

uint32_t crush_font_get_id(struct crush_font *font)
{
        return font->id;
}
// TODO properly fill in these accessors which generate information required by the rendering engine
uint8_t *crush_font_get_target_file(struct crush_font *font)
{
        return font->file[0];
}
uint8_t crush_font_get_target_face_index(struct crush_font *font)
{
        return 0;
}
uint8_t *crush_font_get_name(struct crush_font *font)
{
        return font->name;
}

// shows information about the currently selected CRUSH_FONT, if any
static struct light_cli_invocation_result do_cmd_font(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_add(struct light_cli_invocation *invoke)
{
        struct crush_font *font = light_alloc(sizeof(struct crush_font));
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_remove(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_info(struct light_cli_invocation *invoke)
{
        if(invoke->args_bound < 1) {
                light_error("not enough args bound to run command (%d, requires 1)", invoke->args_bound);
                return Result_Error;
        }
        struct crush_font *font = crush_font_get_by_name(invoke->arg[0]);
        light_info("Crush Font record:");
        light_info("font name: '%s'", font->name);
        light_info("font type: '%s'", font->font_type);
        light_info("font version: '%s'", font->font_version);
        light_info("source URL: '%s'", font->source_url);
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_list(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static void print_usage_font()
{
        printf(
                "Usage:\n"
                "crush font [list] [<options>] \n"
                "crush font info <name> [<options>] \n"
                "crush font add <name> <source-url> [<options>] \n"
                "crush font remove <name> [<options>] \n"
        );
}

static void print_usage_font_add()
{
        printf(
                "Usage:\n"
                "crush font add <name> <path> [<options>] \n"
                "crush font add <name> <source-url> [<options>] \n"
        );
}
static void print_usage_font_remove()
{
        printf(
                "Usage:\n"
                "crush font remove <name> [<options>] \n"
        );
}
static void print_usage_font_info()
{
        printf(
                "Usage:\n"
                "crush font info <name> [<options>] \n"
        );
}
static void print_usage_font_list()
{
        printf(
                "Usage:\n"
                "crush font list [<options>] \n"
        );
}