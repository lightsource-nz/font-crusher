#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CODE_INVALID_ARG            1

static bool have_font_size;
static bool have_out_filename;
static uint8_t font_size;
static char *out_filename;
static char *in_filename = NULL;
static char *out_dirname = NULL;

void print_usage();
void set_option_font_size(char *value);
void set_option_out_filename(char *value);
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
        have_font_size = true;
}
void set_option_out_filename(char *value)
{
        out_filename = value;
        have_out_filename = true;
}
void print_usage()
{
    printf("Usage: font_crusher [-s <size>] [-o <name>] <font_filename> <output_directory>");
}
