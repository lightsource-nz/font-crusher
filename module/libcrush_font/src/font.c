#include <crush.h>

#include <jansson.h>

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

uint8_t crush_font_init(struct light_command *cmd_parent)
{
        crush_common_register_context_object_loader(CRUSH_FONT_CONTEXT_OBJECT_NAME, CRUSH_FONT_CONTEXT_JSON_FILE,
                                        crush_font_create_context, crush_font_load_context);
        return LIGHT_OK;
}
struct crush_json crush_font_create_context()
{
        json_t *display_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:font",
                        "s:[]"                   // "contextFonts"
                "}",
                "version", CRUSH_CONTEXT_JSON_SCHEMA_VERSION, "type", "crush:font", "contextFonts");
        return (struct crush_json) { display_obj };
}
void crush_font_load_context(struct crush_context *context, struct crush_json data)
{
        
}

struct crush_font *crush_font_context_get(struct crush_font_context *context)
{

}
uint8_t crush_font_context_save(struct crush_font_context *context, struct crush_font *font)
{

}

struct crush_json crush_font_object_serialize(struct crush_font *font)
{
        json_t *files = json_array();
        for(uint8_t i = 0; i < font->file_count; i++) {
                json_array_append(files, json_string(font->file[i]));
        }
        json_t *obj = json_pack(
                "{"
                        "s:s,"          //      "name":                 "org.font_name"
                        "s:s"           //      "source_uri":           "git:https//github.com/org/font_name"
                        "s:s,"          //      "version_string":       "0.1.0"
                        "s:s"           //      "mod_root":             "modules/crush.core"
                        "s:o"           //      "files":
                "}",
                "name",         font->name,
                "source_url",   font->source_url,
                "font_type",    font->font_type,
                "files",        &files
                );
        return (struct crush_json) { obj };
}
// ==> struct crush_font *crush_font_object_deserialize(struct crush_json data)
//   we may come back and implement full custom field extraction for objects of differing
// font_type values, but for now all font objects are expected to have the same fields
struct crush_font *crush_font_object_deserialize(struct crush_json data)
{
        struct crush_font *font = light_alloc(sizeof(struct crush_font));
        struct crush_json files_data;
        json_unpack(data.target, 
                "{"
                        "s:s,"          //      "name":                 "font_creator.sans_helvetica"
                        "s:s,"          //      "source_url"            "git:https//github.com/font_creator/sans_helvetica"
                        "s:s,"          //      "font_type"             "crush:font:opentype"
                        "s:s,"          //      "font_version"          "[git commit hash]"
                        "s:s,"          //      "font_root"             "font/font_creator.sans_helvetica"
                        "s:o"           //      "files"
                "}",
                "name",         &font->name,
                "source_url",   &font->source_url,
                "font_type",    &font->font_type,
                "font_type",    &font->font_version,
                "font_root",    &font->path,
                "files",        &files_data.target
        );
        uint8_t i;
        json_t *file_value;
        json_array_foreach(files_data.target, i, file_value) {
                json_unpack(file_value, "s", &font->file[i]);
        }
}

struct light_command *crush_font_get_command()
{
        return &cmd_crush_font;
}
struct light_command *crush_font_get_subcommand_add()
{
        return &cmd_crush_font_add;
}
struct light_command *crush_font_get_subcommand_remove()
{
        return &cmd_crush_font_remove;
}
struct light_command *crush_font_get_subcommand_info()
{
        return &cmd_crush_font_info;
}
struct light_command *crush_font_get_subcommand_list()
{
        return &cmd_crush_font_list;
}

// shows information about the currently selected CRUSH_FONT, if any
static struct light_cli_invocation_result do_cmd_font(struct light_cli_invocation *invoke)
{
        // pull value of CRUSH_FONT environment variable
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_font_add(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_font_remove(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_font_info(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_font_list(struct light_cli_invocation *invoke)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
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