#include <crush.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <freetype2/ft2build.h>
#include <freetype/freetype.h>
#include <jansson.h>

#include "render_private.h"


#define COMMAND_RENDER_NAME                     "render"
#define COMMAND_RENDER_DESCRIPTION              "command used to access text rendering functions, and the data produced by them"
#define COMMAND_RENDER_NEW_NAME                 "new"
#define COMMAND_RENDER_NEW_DESCRIPTION          "command used to render a new set of character glyphs from a font file and screen information"
#define COMMAND_RENDER_INFO_NAME                "info"
#define COMMAND_RENDER_INFO_DESCRIPTION         "command used to view information about a given render data object"
#define COMMAND_RENDER_LIST_NAME                "list"
#define COMMAND_RENDER_LIST_DESCRIPTION         "command used to view information about a given render data object"

static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_render, &cmd_crush, COMMAND_RENDER_NAME, COMMAND_RENDER_DESCRIPTION, do_cmd_render, 0, 0);
Light_Command_Define(cmd_crush_render_new, &cmd_crush_render, COMMAND_RENDER_NEW_NAME, COMMAND_RENDER_NEW_DESCRIPTION, do_cmd_render_new, 2, 3);
Light_Command_Define(cmd_crush_render_info, &cmd_crush_render, COMMAND_RENDER_INFO_NAME, COMMAND_RENDER_INFO_DESCRIPTION, do_cmd_render_info, 0, 1);
Light_Command_Define(cmd_crush_render_list, &cmd_crush_render, COMMAND_RENDER_LIST_NAME, COMMAND_RENDER_LIST_DESCRIPTION, do_cmd_render_list, 1, 2);

static void print_usage_render();
static void print_usage_render_new();
static void print_usage_render_info();
static void print_usage_render_list();

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_RENDER_CONTEXT_OBJECT_NAME

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:[]}"

static FT_Library freetype;

void _render_load_event()
{
        int err;
        if(err = FT_Init_FreeType(&freetype)) {
                light_fatal("failed to initialise the freetype2 typesetting library: FT_Init_FreeType() returned value %d", err);
        }
        int major, minor, patch;
        FT_Library_Version(freetype, &major, &minor, &patch);
        light_debug("loaded freetype2 version %d.%d.%d", major, minor, patch);
}
uint8_t crush_render_init(struct light_command *cmd_parent)
{
        return CODE_OK;
}
crush_json_t *crush_render_create_context(uint8_t *path)
{
        json_t *render_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:render",
                        "s:[]"                  // "contextRenders"
                "}",
                "version", SCHEMA_VERSION, "type", OBJECT_NAME, "contextRenders");

        return render_obj;
}
void crush_render_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_render_context *render_ctx = light_alloc(sizeof(struct crush_render_context));
        render_ctx->root = context;
        render_ctx->file_path = file_path;
        json_unpack(
                data,
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:render",
                        "s:O"                   // "contextRenders"
                "}",
                "version", &render_ctx->version, "type", &type,
                "contextRenders", &render_ctx->data);
        json_decref(data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, CRUSH_MODULE_CONTEXT_OBJECT_NAME, render_ctx);
}
struct crush_render *crush_render_context_get(struct crush_render_context *context, const uint8_t *id)
{
        crush_json_t *obj_data = json_object_get(context->data, id);
        struct crush_font *result = crush_render_object_deserialize(obj_data);
        json_decref(obj_data);
        return result;
}
uint8_t crush_render_context_save(struct crush_render_context *context, const uint8_t *id, struct crush_render *object)
{
        return json_object_set_new(context->data, id, crush_font_object_serialize(object));
}
uint8_t crush_render_context_commit(struct crush_render_context *context)
{       
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT,
                                        "version",              context->version,
                                        "type",                 CRUSH_FONT_CONTEXT_OBJECT_NAME,
                                        "contextRenders",       context->data);
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        json_decref(obj_data);
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);

        return 0;
}

crush_json_t *crush_render_object_serialize(struct crush_render *object)
{
        json_t *data = json_pack(
                "{"
                        "s:s,"          //      "name": "render:sans_helvetica/$disp/$version"
                        "s:s,"          //      "path": "data/render/sans_helvetica/$disp/$version"
                "}",
                "name",         object->name,
                "path",         object->path
                );
        return data;
}
struct crush_render *crush_render_object_deserialize(crush_json_t *data)
{
        struct crush_render *object = light_alloc(sizeof(struct crush_render));
        json_unpack(data, 
                "{"
                        "s:s,"          //      "name":                 "crush:render:$id"
                        "s:s,"          //      "path":                 "$context/data/render/$id"
                "}",
                "name",         &object->name,
                "path",         &object->path
        );
        json_decref(data);
        return object;
}

static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke)
{
        print_usage_render();
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}

static void print_usage_render()
{
        printf(
                "Usage:\n"
                "crush render list \n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
static void print_usage_render_new()
{
        printf(
                "Usage:\n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
static void print_usage_render_info()
{
        printf(
                "Usage:\n"
                "crush render info <render_id> \n"
        );
}
static void print_usage_render_list()
{
        printf(
                "Usage:\n"
                "crush render list \n"
        );
}
