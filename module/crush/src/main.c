#include <crush.h>
#include <module/mod_light_cli.h>
#include <module/mod_libcrush.h>
#include "crush_private.h"

#define CRUSH_ROOT_COMMAND_NAME                 "crush"
#define CRUSH_ROOT_COMMAND_DESCRIPTION          "default root command for the Font Crusher application"

#define CRUSH_VERSION_MAJOR                     1

#define DPI_MAX_DIGITS                          8

static uint8_t font_size = 0;
static uint8_t font_size_unit = UNIT_POINT;
static char *out_filename = NULL;
static char *in_filename = NULL;
static char *out_dirname = NULL;
static uint16_t dpi_h = DPI_H_DEFAULT;
static uint16_t dpi_v = DPI_V_DEFAULT;

static struct light_command *cmd_root;

static void crush_app_event(const struct light_module *mod, uint8_t event, void *arg);
static uint8_t crush_app_main(struct light_application *app);

Light_Application_Define(
        crush, crush_app_event, crush_app_main,
        &libcrush,
        &light_cli,
        &light_core
);

static void crush_init();
static void print_usage_context();
static uint8_t parse_command_name(const char *name);
static uint8_t command_is_container(void *obj);
void set_option_font_size(char *value);
void set_option_out_filename(char *value);
uint8_t set_option_dpi(char *value);
int main(int argc, char *argv[])
{
        light_framework_init(argc, argv);
        light_framework_run();
}
static void crush_app_event(const struct light_module *mod, uint8_t event_id, void *arg)
{
        switch (event_id)
        {
        case LF_EVENT_APP_LOAD:
                struct light_event_app_load *event = (struct light_event_app_load *)arg;
                
                break;
        
        default:
                break;
        }
        
}
static uint8_t crush_app_main(struct light_application *app)
{
        light_debug("enter main task","");

        // default application lifecycle for command executables is run once then shutdown
        return LF_STATUS_SHUTDOWN;
}
static void do_cmd_crush(struct light_command *command){
        print_usage_context();
}
// perform all init activities that take place before the parsing of
// command line arguments, such as loading built-in commands, and
// installed plugin modules
static void crush_init()
{
        cmd_root = light_cli_register_command(
                CRUSH_ROOT_COMMAND_NAME, CRUSH_ROOT_COMMAND_DESCRIPTION, do_cmd_crush);
        crush_cmd_context_init(cmd_root);
        crush_cmd_display_init(cmd_root);
        crush_cmd_font_init(cmd_root);
        crush_cmd_module_init(cmd_root);
        crush_cmd_render_init(cmd_root);
}
uint8_t crush_process_command_line(int argc, char *argv[])
{
        if(argc < 2) {
                print_usage_context();
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
                print_usage_context();
                return CODE_INVALID_ARG;
        }

        return 0;
}
static uint8_t parse_command_name(const char *name)
{
        if(strcmp(name, STR_COMMAND_CONTEXT)) {

        }
}
void set_option_font_size(char *value)
{
        size_t length = strlen(value);
        char *end;
        font_size = (uint8_t) strtol(value, &end, 10);
        if(end < value + length) {
                if(strcmp(end, "px")) {
                        font_size_unit = UNIT_PIXEL;
                } else if(strcmp(end, "pt")) {
                        font_size_unit = UNIT_POINT;
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
static void print_usage_context()
{
    printf("Usage: font_crusher export [-s <size>] [-o <name>] <font_filename> <output_directory>\n");
}
