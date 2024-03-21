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

struct crush_command {
        uint8_t type;
        uint8_t (*do_cmd)(int argc, char **argv);
};
struct crush_container {
        uint8_t type;
        void *child[CRUSH_SUBCOMMAND_MAX];
};
static struct {
        const char *name;
        void *command;
} command_table[CRUSH_COMMAND_MAX];
static uint8_t next_command_id;

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
// perform all init activities that take place before the parsing of
// command line arguments, such as loading built-in commands, and
// installed plugin modules
static void crush_init()
{
        next_command_id = 0;
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
uint8_t crush_command_container_define(uint8_t parent, const char *name)
{
        uint8_t id = next_command_id++;
        struct crush_container *cmd = malloc(sizeof(struct crush_container));
        cmd->type = TYPE_CONTAINER;
        command_table[id].name = name;
        command_table[id].command = cmd;
        return CODE_OK;
}
uint8_t crush_command_define(uint8_t parent, const char *name, uint8_t (*do_cmd)(int argc, char **argv))
{
        uint8_t id = next_command_id++;
        struct crush_command *cmd = malloc(sizeof(struct crush_command));
        cmd->type = TYPE_COMMAND;
        cmd->do_cmd = do_cmd;
        command_table[id].name = name;
        command_table[id].command = cmd;
        return CODE_OK;
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
