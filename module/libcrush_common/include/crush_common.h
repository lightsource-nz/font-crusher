#ifndef _CRUSH_COMMON_H
#define _CRUSH_COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <light.h>

#include <limits.h>
// NOTE we need to enforce a hard limit on path length for security reasons, but the
// exact value of that limit is less crucial. Most platforms can simply use the
// limit defined by POSIX as PATH_MAX, otherwise use the default value of 4k
#ifdef PATH_MAX
#       define CRUSH_MAX_PATH_LENGTH            PATH_MAX
#else
#       define CRUSH_MAX_PATH_LENGTH            4096
#endif

#define CRUSH_CONTEXT_VARNAME "CRUSH_CONTEXT"

#define Crush_Path_Join_Static(path0, path1) path0 "/" path1

#define CRUSH_CONTEXT_JSON_SCHEMA_VERSION       0
// TODO make sure hardcoded paths work on wind cows etc
#define DOTCRUSH                                ".crush"
#define CRUSH_CONTEXT_PATH_LOCAL                "."
#define CRUSH_CONTEXT_PATH_DEFAULT              "~"
#define CRUSH_CONTEXT_DIR_NAME                  DOTCRUSH
#define CRUSH_CONTEXT_OBJECT_NAME               "crush:context"
#define CRUSH_CONTEXT_JSON_FILE                 "context.json"
#define CRUSH_CONTEXT_OBJECTS_MAX               16

struct crush_context_object {
        uint8_t *name;
        void *object;
};
struct crush_context {
        uint16_t schema_version;
        struct crush_context *parent;
        uint8_t *path;
        uint8_t object_count;
        struct crush_context_object object[CRUSH_CONTEXT_OBJECTS_MAX];
};

struct crush_json {
        void *target;
};

// called automatically by light framework at module load-time
extern void crush_common_init();
// called automatically by light framework after all modules are loaded
extern void crush_common_load_context();
extern void crush_common_register_context_object_loader(uint8_t *name, uint8_t *filename, struct crush_json (*create)(), void (*load)(struct crush_context *, struct crush_json));
// crush application context API
extern bool crush_context_try_load_from_path(uint8_t *path, struct crush_context *context);
extern void crush_context_create_under_path(uint8_t *path, struct crush_context *context);
extern void crush_context_add_context_object(struct crush_context *context, uint8_t *name, void *object);
extern void *crush_context_get_context_object(struct crush_context *context, uint8_t *name);
// filesystem paths API
extern uint8_t *crush_path_join(uint8_t *path0, uint8_t *path1);
// NOTE the caller must always ensure that n is equal to the number of variadic arguments
// passed to the function, or bad things will happen
extern uint8_t *crush_path_join_n(uint8_t n, ...);

#define crush_context_get_context_object_type(_context, _name, _type) \
        (_type) crush_context_get_context_object(_context, _name)

#endif
