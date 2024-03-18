#include <crush.h>

#define DPI_NUM_DIGITS                        8

static uint8_t font_size = 0;
static uint8_t font_size_unit = UNIT_PIXEL;
static char *out_filename = NULL;
static char *in_filename = NULL;
static char *out_dirname = NULL;
static uint16_t dpi_h = DPI_H_DEFAULT;
static uint16_t dpi_v = DPI_V_DEFAULT;

void print_usage();
void set_option_font_size(char *value);
void set_option_out_filename(char *value);
uint8_t set_option_dpi(char *value);
int main(int argc, char *argv[])
{
        if(argc < 2) {
                print_usage();
                return 1;
        }
        // argv[0] is the command name (i.e. font_crusher)
        // argv[1] is the subcommand name, currently there is only one SC
        // so we start parsing options and args from argv[2]
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
void set_option_font_size(char *value)
{
        size_t length = strlen(value);
        char *end;
        font_size = (uint8_t) strtol(value, &end, 10);
        if(end < value + length) {
                printf("invalid number passed to option -s\n");
                exit(CODE_INVALID_ARG);
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
        dpi_h = strtol(val_dpi_h, NULL, 10);
        dpi_v = strtol(val_dpi_v, NULL, 10);
        return CODE_OK;
}
void print_usage()
{
    printf("Usage: font_crusher export [-s <size>] [-o <name>] <font_filename> <output_directory>\n");
}
