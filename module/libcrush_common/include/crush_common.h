#ifndef _CRUSH_COMMON_H
#define _CRUSH_COMMON_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

#include <light.h>

#include <jansson.h>

#include <threads.h>
#include <stdatomic.h>

#define CRUSH_OK                0
#define CRUSH_ERR_QUEUE         1

#include <crush_queue.h>

// NOTE we need to enforce a hard limit on path length for security reasons, but the
// exact value of that limit is less crucial. Most platforms can simply use the
// limit defined by POSIX as PATH_MAX, otherwise use the default value of 4k
#ifdef PATH_MAX
#       define CRUSH_MAX_PATH_LENGTH            PATH_MAX
#else
#       define CRUSH_MAX_PATH_LENGTH            4096
#endif

#define CRUSH_JSON_LPRIME                       3751146389
#define CRUSH_JSON_INCREMENT                    (INT_MAX - CRUSH_JSON_LPRIME)
#define CRUSH_JSON_ID_NEW                       CRUSH_JSON_LPRIME
#define ID_To_String(sym, id) \
                uint8_t sym[CRUSH_JSON_KEY_LENGTH]; \
                snprintf(id_str, CRUSH_JSON_KEY_LENGTH, "%8X", id);
#define String_To_ID(string) strtoul(string, NULL, 16)
// object IDs are a hex conversion of a 32-bit counter value, 8 byte fixed-length strings
#define CRUSH_JSON_KEY_LENGTH                   8

#define CRUSH_EV_CONTEXT                        "CRUSH_CONTEXT"
#define CRUSH_EV_FONT                           "CRUSH_FONT"
#define CRUSH_EV_DISPLAY                        "CRUSH_DISPLAY"

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
        uint8_t *filename;
        void *object;
};
struct crush_context {
        uint16_t schema_version;
        struct crush_context *parent;
        uint8_t *path;
        uint8_t object_count;
        struct crush_context_object object[CRUSH_CONTEXT_OBJECTS_MAX];
};

typedef json_t crush_json_t;

#ifndef asprintf
#define CRUSH_ASPRINTF
int vasprintf(char **strp, const char *fmt, va_list ap);

int asprintf(char **strp, const char *fmt, ...);
#endif

// called automatically by light framework at module load-time
extern void crush_common_init();
// called automatically by light framework after all modules are loaded
extern void crush_common_load_context();

extern void crush_common_register_context_object_loader(const uint8_t *name, const uint8_t *filename, crush_json_t *(*create)(), void (*load)(struct crush_context *, const uint8_t *, crush_json_t *));
extern uint32_t crush_common_get_initial_counter_value();
extern uint32_t crush_common_get_next_counter_value(uint32_t value);
extern uint8_t *crush_common_datetime_string();
// crush application context API
extern struct crush_context *crush_context();
extern bool crush_context_try_load_from_path(uint8_t *path, struct crush_context *context);
extern void crush_context_create_under_path(uint8_t *path, struct crush_context *context);
extern void crush_context_add_context_object(struct crush_context *context, uint8_t *name, void *object);
extern void *crush_context_get_context_object(struct crush_context *context, uint8_t *name);
// filesystem paths API
extern uint8_t *crush_path_join(const uint8_t *path0, const uint8_t *path1);
// NOTE the caller must always ensure that n is equal to the number of variadic arguments
// passed to the function, or bad things will happen
extern uint8_t *crush_path_join_n(uint8_t n, ...);

#define crush_context_get_context_object_type(_context, _name, _type) \
        (_type) crush_context_get_context_object(_context, _name)

#endif
