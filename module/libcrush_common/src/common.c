#ifdef __GNUC__
#define _GNU_SOURCE
#endif
#include <crush_common.h>
#include <jansson.h>

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

static void crush_load_context_from_filesystem();
void crush_common_init()
{
        light_debug("jansson lib version %s", jansson_version_str());
        json_set_alloc_funcs(light_alloc, light_free);
        crush_load_context_from_filesystem();
}

// -- context loader routine
// 1: locate context root by attempting to load 'context.json' from different paths
// - try $env[CRUSH_CONTEXT] if it exists
// - search for a '.crush' directory under CWD (?)
// - fall back to default context under user home, if this context does not already exist
//   then we attempt to create it before loading
static void crush_load_context_from_filesystem()
{
#ifdef __GNUC__
        uint8_t *context_path = secure_getenv(CRUSH_CONTEXT_VARNAME);
#else
        uint8_t *context_path = getenv(CRUSH_CONTEXT_VARNAME);
#endif
        if(!context_path) {
                DIR *dir_p = opendir(CRUSH_CONTEXT_PATH_LOCAL);
                if(dir_p) {
                        //struct dirent *dir_ent = readdir(dir_p);

                } else {
                        context_path = CRUSH_CONTEXT_DEFAULT_PATH;

                }
        }
}
#define CRUSH_CONTEXT_DIR_NAME                  ".crush"
#define CRUSH_CONTEXT_JSON_FILE                 "context.json"

// at this stage, we test for the existence of a context by verifying that we can open a handle to
// ${context}/context.json; if the json file does not contain a well-formed crush context, this is
// treated as an error
struct crush_context *crush_context_try_load_from_path(uint8_t *path)
{
        uint8_t *ctx_file_path = crush_path_join(path, Crush_Path_Join_Static(CRUSH_CONTEXT_DIR_NAME, CRUSH_CONTEXT_JSON_FILE));
        FILE *ctx_file = fopen(ctx_file_path, "r");
        if(!ctx_file) {
                // failure to find a context is not an error, just return NULL
                light_debug("no crush context found at filesystem path '%s'", ctx_file_path);
                light_free(ctx_file_path);
                return NULL;
        }
        fclose(ctx_file);
        json_error_t err;
        struct crush_json context_root_json = { json_load_file(ctx_file_path, 0, &err) };
        light_free(ctx_file_path);

        // we have loaded our context root as a json object, ready to be parsed
        struct crush_context *context = light_alloc(sizeof(struct crush_context));
        if(!context) {
                light_fatal("failed to allocate memory for context object");
        }

        return context;
}
// NOTE this routine assumes that all input paths are normalized, and path0 specifically is
// the path to a directory with no trailing path separator
// NOTE this routine allocates heap memory for the combined path, and the caller is 
// responsible for making sure it is freed
uint8_t *crush_path_join(uint8_t *path0, uint8_t *path1)
{
        // len(out) == len(p0) + '/' + len(p1) + '\0'
        //          == len(p0+p1) + 2
        // one extra char for path separator, one for terminating null char
        uint16_t total_length = 2;
        total_length += strlen(path0);
        total_length += strlen(path1);
        if(total_length > CRUSH_MAX_PATH_LENGTH) {
                light_warn("combined path exceeds maximum path length (%d)", total_length);
                return NULL;
        }

        // TODO this requires adding a calloc proxy routine to the light_common api
        uint8_t *out = calloc(sizeof(uint8_t), total_length);
        strcat(out, path0);
        strcat(out, "/");
        strcat(out, path1);
        return out;
}
// doing paths this way is a greasy hack, but recursively joining paths one by one
// has unacceptably bad performance and should be strictly avoided
// NOTE the caller must always ensure that n is equal to the number of variadic arguments
// passed to the function, or bad things will happen
extern uint8_t *crush_path_join_n(uint8_t n, ...)
{
        if(n < 2) {
                light_warn("invalid argument: requires at least 2 args (%d given)", n);
                return NULL;
        }
        uint8_t **arg = calloc(n, sizeof(uint8_t *));
        // output string contains (n - 1) separators and 1 null char
        uint16_t total_length = n;
        va_list args;
        va_start(args, n);
        for(uint8_t i = 0; i < n; i++) {
                arg[i] = va_arg(args, uint8_t *);
                total_length += strlen(arg[i]);
        }
        va_end(args);
        if(total_length > CRUSH_MAX_PATH_LENGTH) {
                light_warn("combined path exceeds maximum path length (%d)", total_length);
                light_free(arg);
                return NULL;
        }
        uint8_t *out = calloc(sizeof(uint8_t), total_length);
        for(uint8_t i = 0; i < n; i++) {
                if(i > 0) strcat(out, "/");
                strcat(out, arg[i]);
        }
        light_free(arg);
        return out;
}
