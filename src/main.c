#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#define CODE_INVALID_ARG                1

#define UNIT_PIXEL                      0
#define UNIT_POINT                      1
#define DPI_H_DEFAULT                   300
#define DPI_V_DEFAULT                   300

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
void set_option_dpi(uint16_t val_dpi_h, uint16_t val_dpi_v);
int main(int argc, char *argv[])
{
        if(argc < 2) {
                print_usage();
                return 1;
        }
        for(uint8_t i = 0; i < argc; i++) {
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
void set_option_dpi(uint16_t val_dpi_h, uint16_t val_dpi_v)
{
        dpi_h = val_dpi_h;
        dpi_v = val_dpi_v;
}
void print_usage()
{
    printf("Usage: font_crusher [-s <size>] [-o <name>] <font_filename> <output_directory>");
}
