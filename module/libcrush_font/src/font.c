#include <crush.h>

#define COMMAND_FONT_NAME "font"
#define COMMAND_FONT_DESCRIPTION "main subcommand for access to the crush font database"
#define COMMAND_FONT_ADD_NAME "add"
#define COMMAND_FONT_ADD_DESCRIPTION "adds an entry to the local font database, from a web URL or local file"
#define COMMAND_FONT_REMOVE_NAME "remove"
#define COMMAND_FONT_REMOVE_DESCRIPTION "removes an existing entry from the local crush font database"

static struct light_command *cmd_font;
static struct light_command *cmd_font_add;
static struct light_command *cmd_font_remove;

static void print_usage_font();
static void print_usage_font_add();
static void print_usage_font_remove();
static void do_cmd_font(struct light_command *command);
static void do_cmd_font_add(struct light_command *command);
static void do_cmd_font_remove(struct light_command *command);
uint8_t crush_font_init(struct light_command *cmd_parent)
{
        /*
        cmd_font = light_cli_register_subcommand(cmd_parent,
                COMMAND_FONT_NAME, COMMAND_FONT_DESCRIPTION, do_cmd_font);
        cmd_font_add = light_cli_register_subcommand(cmd_font,
                COMMAND_FONT_ADD_NAME, COMMAND_FONT_ADD_DESCRIPTION, do_cmd_font_add);
        cmd_font_remove = light_cli_register_subcommand(cmd_font,
                COMMAND_FONT_REMOVE_NAME, COMMAND_FONT_REMOVE_DESCRIPTION, do_cmd_font_remove);
        */

        return LIGHT_OK;
}
struct light_command *crush_font_get_command()
{
        return cmd_font;
}
struct light_command *crush_font_get_subcommand_add()
{
        return cmd_font_add;
}
struct light_command *crush_font_get_subcommand_remove()
{
        return cmd_font_remove;
}

// shows information about the currently selected CRUSH_FONT, if any
static void do_cmd_font(struct light_command *command)
{
        // pull value of CRUSH_FONT environment variable
        
}
static void do_cmd_font_add(struct light_command *command)
{
        
}
static void do_cmd_font_remove(struct light_command *command)
{

}
static void print_usage_font()
{
        printf(
                "Usage:\n"
                "crush font \n"
                "crush font add <name> <source_url> \n"
                "crush font remove name \n"
        );
}

static void print_usage_font_new()
{
        printf(
                "Usage:\n"
                "crush font new [<ctx_path>] \n"
        );
}
static void print_usage_font_set()
{
        printf(
                "Usage:\n"
                "crush font set [<ctx_path>] \n"
        );
}