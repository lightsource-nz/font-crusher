#include <crush.h>

#include <light_cli.h>
#include <jansson.h>

#define COMMAND_DISPLAY_NAME                    "display"
#define COMMAND_DISPLAY_DESCRIPTION             "command used to interact with entries in the crush display database"
#define COMMAND_DISPLAY_IMPORT_NAME             "import"
#define COMMAND_DISPLAY_IMPORT_DESCRIPTION      "command used to add new display device entries to the local database"
#define COMMAND_DISPLAY_INFO_NAME               "info"
#define COMMAND_DISPLAY_INFO_DESCRIPTION        "command used to show information about a specific display device entry"
#define COMMAND_DISPLAY_LIST_NAME               "list"
#define COMMAND_DISPLAY_LIST_DESCRIPTION        "command used to list all entries in the display database"

static struct light_command *cmd_display;
static struct light_command *cmd_display_import;
static struct light_command *cmd_display_info;
static struct light_command *cmd_display_list;

static void print_usage_context();
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command);

Light_Command_Define(cmd_crush_display, &cmd_crush, COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display, 0, 0);
Light_Command_Define(cmd_crush_display_import, &cmd_crush_display, COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import, 1, 1);
Light_Command_Define(cmd_crush_display_info, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);
Light_Command_Define(cmd_crush_display_list, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_DISPLAY_CONTEXT_OBJECT_NAME
#define JSON_FILE CRUSH_DISPLAY_CONTEXT_JSON_FILE

uint8_t crush_display_init()
{
        crush_common_register_context_object_loader(OBJECT_NAME, JSON_FILE,
                                        crush_display_create_context, crush_display_load_context);
        /*
        cmd_display = light_cli_register_subcommand(cmd_parent,
                COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display);
        cmd_display_import = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import);
        cmd_display_info = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info);
        */
       
        return CODE_OK;
}
crush_json_t *crush_display_create_context()
{
        json_t *display_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:display",
                        "s:[]"                  // "contextDisplays"
                "}",
                "version",      SCHEMA_VERSION,
                "type",         OBJECT_NAME,
                "contextDisplays");
                return display_obj;
}
void crush_display_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *json)
{
        uint8_t *type;
        struct crush_display_context *display_context = light_alloc(sizeof(struct crush_display_context));
        display_context->root = context;
        display_context->file_path = file_path;
        json_unpack(json,
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              "crush:font",
                        "s:o"                   // "contextFonts"
                "}",
                "version", &display_context->version, "type", &type,
                "contextFonts", &display_context->data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, OBJECT_NAME, (void *)display_context);
}
struct crush_display *crush_display_context_get(struct crush_display_context *context, const uint8_t *id)
{
        uint8_t *name, *description;
        uint16_t res_h, res_v, ppi_h, ppi_v;
        double height_mm, width_mm;
        json_unpack(context->data, "{s:{s:{s:s,s?:s,s:s,s:s,s:s,s:s,s?:s,s?:s}}}", "displayObjects",
                id, "name", &name, "description", &description, "res_h", &res_h, "res_v", &res_v,
                "ppi_h", &ppi_h, "ppi_v", &ppi_v, "height_mm", &height_mm, "width_mm", &width_mm);
        struct crush_display *display = light_alloc(sizeof(struct crush_display));
        display->name = name;
        display->description = description;
        display->resolution_h = res_h;
        display->resolution_v = res_v;
        display->ppi_h = ppi_h;
        display->ppi_v = ppi_v;
        return display;
}
uint8_t crush_display_context_save(struct crush_display_context *context, const uint8_t *id, struct crush_display *object)
{
        
}
uint8_t crush_display_context_commit(struct crush_display_context *context)
{

}

crush_json_t *crush_display_object_serialize(struct crush_display *font)
{

}
struct crush_display *crush_display_object_deserialize(crush_json_t *data)
{

}

struct light_command *crush_display_get_command()
{
        return cmd_display;
}
struct light_command *crush_display_get_subcommand_import()
{
        return cmd_display_import;
}
struct light_command *crush_display_get_subcommand_info()
{
        return cmd_display_info;
}
static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush display info <display_id> \n"
                "crush display import <import_path> \n"
        );
}
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command)
{
        print_usage_context();
}
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command)
{
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
