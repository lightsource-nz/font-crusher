#ifndef _CRUSH_H
#define _CRUSH_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ft2build.h>
#include <freetype/freetype.h>

#define CODE_OK                         0
#define CODE_INVALID_ARG                1

#define UNIT_PIXEL                      0
#define UNIT_POINT                      1
#define DPI_H_DEFAULT                   300
#define DPI_V_DEFAULT                   300

uint8_t command_export();

#endif
