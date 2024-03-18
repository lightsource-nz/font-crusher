#include <crush.h>

void print_usage();
uint8_t command_export()
{
        
        return CODE_OK;
}
void print_usage()
{
    printf("Usage: crush export [-s <size>] [-o <name>] <font_filename> <output_directory>\n");
}
