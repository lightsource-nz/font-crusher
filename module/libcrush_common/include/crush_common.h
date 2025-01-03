#ifndef _CRUSH_COMMON_H
#define _CRUSH_COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <light.h>

#define CRUSH_CONTEXT_VARNAME "CRUSH_CONTEXT"
#define CRUSH_CONTEXT_DEFAULT_PATH "~/.crush"
#define CRUSH_CONTEXT_ROOT_JSON_FILE "context.json"

#define Crush_Path_Join_Static(path0, path1) path0 "/" path1

struct crush_context {
        struct crush_context *parent;
        uint8_t *path;
};

struct crush_json {
        void *target;
};

// called automatically by light framework at module load-time
extern void crush_common_init();

// crush application context API
extern struct crush_context *crush_context_try_load_from_path(uint8_t *path);

// filesystem paths API
extern uint8_t *crush_path_join(uint8_t *path0, uint8_t *path1);
extern uint8_t *crush_path_join_n(uint8_t n, ...);

#endif
