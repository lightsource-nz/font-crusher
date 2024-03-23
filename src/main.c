#include <crush.h>
#include "crush_private.h"

#define DPI_MAX_DIGITS                        8

static uint8_t font_size = 0;
static uint8_t font_size_unit = UNIT_POINT;
static char *out_filename = NULL;
static char *in_filename = NULL;
static char *out_dirname = NULL;
static uint16_t dpi_h = DPI_H_DEFAULT;
static uint16_t dpi_v = DPI_V_DEFAULT;

static struct crush_command *command_table[CRUSH_COMMAND_MAX];
static uint8_t next_command_id;

static struct crush_container *cmd_root;

static void crush_init();
static void print_usage();
static uint8_t parse_command_name(const char *name);
static uint8_t command_is_container(void *obj);
void set_option_font_size(char *value);
void set_option_out_filename(char *value);
uint8_t set_option_dpi(char *value);
int main(int argc, char *argv[])
{
        if(argc < 2) {
                print_usage();
                return 1;
        }
        // argv[0] is the command name (i.e. crush)
        // argv[1] is the subcommand name (font, render, etc)
        // so we start iteratively parsing args from argv[2]
        for(uint8_t i = 2; i < argc; i++) {
                if(argv[i][0] == '-') {
                        switch (argv[i++][1]) {
                        case 's':
                                set_option_font_size(argv[i]);
                                break;
                        case 'o':
                                set_option_out_filename(argv[i]);
                                break;
                        }
                } else if(!in_filename) {
                        in_filename = argv[i];
                } else if(!out_dirname) {
                        out_dirname = argv[i];
                }
        }
        if(!out_dirname) {
                printf("Error: too few arguments (expected 2)\n");
                print_usage();
                return CODE_INVALID_ARG;
        }

        return 0;
}
static uint8_t do_cmd_crush(int argc, char **argv){
        print_usage();
        return CODE_OK;
}
// perform all init activities that take place before the parsing of
// command line arguments, such as loading built-in commands, and
// installed plugin modules
static void crush_init()
{
        next_command_id = 0;
        cmd_root = crush_command_container_define(NULL, "crush", do_cmd_crush);
        crush_cmd_context_init();
        crush_cmd_display_init();
        crush_cmd_font_init();
        crush_cmd_module_init();
        crush_cmd_render_init();
}
static uint8_t parse_command_name(const char *name)
{
        if(strcmp(name, STR_COMMAND_CONTEXT)) {

        }
}
struct crush_container *crush_command_container_define(struct crush_container *parent, const char *name, uint8_t (*do_cmd)(int argc, char **argv))
{
        uint8_t id = next_command_id++;
        struct crush_container *cmd = malloc(sizeof(struct crush_container));
        cmd->type = TYPE_CONTAINER;
        cmd->name = name;
        cmd->do_cmd = do_cmd;
        command_table[id] = (struct crush_command *) cmd;
        cmd->child_count = 0;
        if(id != CRUSH_COMMAND_PARENT_NONE) {
                crush_command_container_add(parent, (struct crush_command *) cmd);
        }
        return cmd;
}
struct crush_command *crush_command_define(struct crush_container *parent, const char *name, uint8_t (*do_cmd)(int argc, char **argv))
{
        uint8_t id = next_command_id++;
        struct crush_command *cmd = malloc(sizeof(struct crush_command));
        cmd->type = TYPE_COMMAND;
        cmd->name = name;
        cmd->do_cmd = do_cmd;
        command_table[id] = cmd;
        if(id != CRUSH_COMMAND_PARENT_NONE) {
                crush_command_container_add(parent, cmd);
        }
        return cmd;
}
uint8_t crush_command_container_add(struct crush_container *parent, struct crush_command *child)
{
        if(parent->child_count >= CRUSH_SUBCOMMAND_MAX) {
                printf("warning: " __FUNCTION__ ": command '%s' max subcommands reached", crush_command_get_path((struct crush_command *)parent));
                return CODE_NO_RESOURCE;
        }
        parent->child[parent->child_count++] = child;
        return CODE_OK;
}
uint8_t crush_num_commands_defined()
{
        return next_command_id;
}
const char *crush_command_get_name(struct crush_command *command)
{
        return command->name;
}
// FIXME this scales horribly and generally wastes memory
const char *crush_command_get_path(struct crush_command *command)
{
        if(command->parent) {
                return strcat(crush_command_get_path((struct crush_command *)command->parent), command->name);
        }
        return command->name;
}
struct crush_command *crush_command_find(struct crush_container *parent, const char *name)
{
        for(uint8_t i = 0; i < parent->child_count; i++) {
                if(strcmp(name, parent->child[i]->name))
                        return parent->child[i];
        }
        return NULL;
}
uint8_t crush_command_exec(struct crush_command *command, int argc, char **argv)
{
        return command->do_cmd(argc, argv);
}
void set_option_font_size(char *value)
{
        size_t length = strlen(value);
        char *end;
        font_size = (uint8_t) strtol(value, &end, 10);
        if(end < value + length) {
                if(strcmp(end, "px")) {
                        font_size_unit = UNIT_PIXEL;
                } else {
                        printf("invalid number passed to option -s\n");
                        exit(CODE_INVALID_ARG);
                }
        }
}
struct crush_container *crush_command_root()
{
        return cmd_root;
}
void set_option_out_filename(char *value)
{
        out_filename = value;
}
uint8_t set_option_dpi(char *value)
{
        uint8_t x = 0;
        while(value[x] != 'x' && value[x] != 'X') {
                if(value[x] == 0) return CODE_INVALID_ARG;
                if(x > DPI_MAX_DIGITS) return CODE_INVALID_ARG;
                x++;

        }
        uint8_t length = (uint8_t) strlen(value);
        char val_dpi_h[DPI_MAX_DIGITS];
        char val_dpi_v[DPI_MAX_DIGITS];
        memcpy(val_dpi_h, value, length - x);
        memcpy(val_dpi_v, &value[x + 1], length - (length - x));
        dpi_h = (uint16_t) strtol(val_dpi_h, NULL, 10);
        dpi_v = (uint16_t) strtol(val_dpi_v, NULL, 10);
        return CODE_OK;
}
static void print_usage()
{
    printf("Usage: font_crusher export [-s <size>] [-o <name>] <font_filename> <output_directory>\n");
}
