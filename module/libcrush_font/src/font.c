#include <crush.h>

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
static void do_cmd_font(struct light_command *command);
static void do_cmd_font_add(struct light_command *command);
static void do_cmd_font_remove(struct light_command *command);
static void do_cmd_font_info(struct light_command *command);
static void do_cmd_font_list(struct light_command *command);

Light_Subcommand_Define(cmd_crush_font, &cmd_crush, COMMAND_FONT_NAME, COMMAND_FONT_DESCRIPTION, do_cmd_font);
Light_Subcommand_Define(cmd_crush_font_add, &cmd_crush_font, COMMAND_FONT_ADD_NAME, COMMAND_FONT_ADD_DESCRIPTION, do_cmd_font_add);
Light_Subcommand_Define(cmd_crush_font_remove, &cmd_crush_font, COMMAND_FONT_REMOVE_NAME, COMMAND_FONT_REMOVE_DESCRIPTION, do_cmd_font_remove);
Light_Subcommand_Define(cmd_crush_font_info, &cmd_crush_font, COMMAND_FONT_INFO_NAME, COMMAND_FONT_INFO_DESCRIPTION, do_cmd_font_info);
Light_Subcommand_Define(cmd_crush_font_list, &cmd_crush_font, COMMAND_FONT_LIST_NAME, COMMAND_FONT_LIST_DESCRIPTION, do_cmd_font_list);

uint8_t crush_font_init(struct light_command *cmd_parent)
{
        return LIGHT_OK;
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
static void do_cmd_font_info(struct light_command *command)
{

}
static void do_cmd_font_list(struct light_command *command)
{

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