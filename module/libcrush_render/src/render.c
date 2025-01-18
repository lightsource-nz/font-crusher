#include <crush.h>

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

struct crush_render_context {
        uint16_t version;
        struct crush_json data;
};

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
struct crush_json crush_render_create_context(uint8_t *path)
{
        uint8_t *render_json_file = crush_path_join(path, "render.json");
        json_t *render_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:render",
                        "s:[]"                   // "contextRenders"
                "}",
                "version", CRUSH_CONTEXT_JSON_SCHEMA_VERSION, "type", CRUSH_RENDER_CONTEXT_OBJECT_NAME, "contextRenders");
        
        json_dump_file(render_obj, render_json_file, JSON_INDENT(8) | JSON_ENSURE_ASCII);
        light_free(render_json_file);
        uint8_t *render_dir = crush_path_join(path, "render");
        if(mkdir(render_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                light_fatal("could not create context module directory at path '%s', mkdir() error code '0x%x'", render_dir, errno);
        }
        light_free(render_dir);
        return (struct crush_json) { render_obj };
}
void crush_render_object_create()
{
        
                                "{"
                                        "s:s,"          //      "name":                 "font_creator.core"
                                        "s:i,"          //      "version_seq"           "0"
                                        "s:s,"          //      "version_string"        "0.1.0"
                                        "s:s"           //      "mod_root"              "modules/crush.core"
                                        "s:[]"          //      "dependencies"
                                "}";
}
void crush_render_load_context(struct crush_context *context, struct crush_json data)
{
        uint8_t *type;
        struct crush_render_context *render_ctx = light_alloc(sizeof(struct crush_render_context));
        json_unpack(
                data.target,
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:module",
                        "s:o"                   // "contextModules"
                "}",
                "version", &render_ctx->version, "type", &type,
                "contextModules", &render_ctx->data);
        crush_context_add_context_object(context, CRUSH_MODULE_CONTEXT_OBJECT_NAME, render_ctx);
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
