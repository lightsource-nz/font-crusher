#ifdef __GNUC__
#define _GNU_SOURCE
#endif
#include <crush_common.h>
#include <jansson.h>

#include <time.h>

#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#define LOADER_MAX              16

struct object_loader {
        const uint8_t *name;
        const uint8_t *filename;
        crush_json_t *(*create)();
        void (*load)(struct crush_context *, const uint8_t *, crush_json_t *);
};

// alias long constant names with shorter local ones
#define SCHEMA_VERSION  CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME     CRUSH_CONTEXT_OBJECT_NAME
#define JSON_FILE       CRUSH_CONTEXT_JSON_FILE

#ifdef CRUSH_ASPRINTF
int vasprintf(char **strp, const char *fmt, va_list ap)
{
        va_list ap1;
        int len;
        char *buffer;
        int res;
    
        va_copy(ap1, ap);
        len = vsnprintf(NULL, 0, fmt, ap1);
    
        if (len < 0)
                return len;
    
        va_end(ap1);
        buffer = light_alloc(len + 1);
    
        if (!buffer)
                return -1;
    
        res = vsnprintf(buffer, len + 1, fmt, ap);
    
        if (res < 0)
                light_free(buffer);
        else
                *strp = buffer;
    
        return res;
}

int asprintf(char **strp, const char *fmt, ...)
{
    int error;
    va_list ap;

    va_start(ap, fmt);
    error = vasprintf(strp, fmt, ap);
    va_end(ap);

    return error;
}
#endif

static struct object_loader loader[LOADER_MAX]; 
static uint8_t next_loader;
static struct crush_context *current_context;

static struct crush_context _root_context;
static struct crush_context *root_context = &_root_context;

static struct crush_context *crush_load_context_from_filesystem(struct crush_context *context);
void crush_common_init()
{
        light_debug("jansson lib version %s", jansson_version_str());
        json_set_alloc_funcs(light_alloc, light_free);
}
void crush_common_load_context()
{
        current_context = root_context = crush_load_context_from_filesystem(root_context);
}
void crush_common_register_context_object_loader(const uint8_t *name, const uint8_t *filename, crush_json_t *(*create)(uint8_t *), void (*load)(struct crush_context *, const uint8_t *, crush_json_t *))
{
        if(next_loader > LOADER_MAX) {
                light_fatal("could not register loader '%s', maximum number of object loaders reached (%i)", name, LOADER_MAX);
        }
        loader[next_loader++]= (struct object_loader) {.name = name, .filename = filename, .create = create, .load = load};
}
uint32_t crush_common_get_initial_counter_value()
{
        return rand() % (CRUSH_JSON_LPRIME + 1);
}
uint32_t crush_common_get_next_counter_value(uint32_t value)
{
        return (value + CRUSH_JSON_INCREMENT) % CRUSH_JSON_LPRIME;
}
#define DATE_STR_SIZE 18
uint8_t *crush_common_datetime_string()
{
        uint8_t *out = light_alloc(DATE_STR_SIZE);
        time_t time_now = time(NULL);
        strftime(out, DATE_STR_SIZE, "%d/%m/%y-%X", localtime(&time_now));
        return out;
}
struct crush_context *crush_context()
{
        return current_context;
}
// -- context loader routine
// 1: locate context root by attempting to load 'context.json' from different paths
// - try $env[CRUSH_CONTEXT] if it exists
// - search for a '.crush' directory under CWD (?)
// - fall back to default context under user home, if this context does not already exist
//   then we attempt to create it before loading
static struct crush_context *crush_load_context_from_filesystem(struct crush_context *context)
{
        uint8_t *context_root, *context_path, *context_file_path;
        bool succeeded = false;
        // first, we try the path indicated by environment variable ${CRUSH_CONTEXT}.
        // the value of ${CRUSH_CONTEXT}, if it is defined, is authoritative; all other search
        // paths are merely fallbacks if ${CRUSH_CONTEXT} is not set
#ifdef __GNUC__
        context_root = secure_getenv(CRUSH_EV_CONTEXT);
#else
        context_root = getenv(CRUSH_EV_CONTEXT);
#endif
        if(context_root) {
                light_info("${CRUSH_CONTEXT} is set to '%s'", context_root);
                context_root = realpath(context_root, NULL);
                context_path = crush_path_join(context_root, DOTCRUSH);
                light_free(context_root);
                context_file_path = crush_path_join(context_path, CRUSH_CONTEXT_JSON_FILE);
                light_free(context_path);
                succeeded = crush_context_try_load_from_path(context_file_path, context);
                if(!succeeded)
                        light_fatal("${CRUSH_CONTEXT} environment variable does not point to a valid crush context (%s)", context_path);
                light_free(context_file_path);
                return context;
        } else {
                light_debug("${CRUSH_CONTEXT} environment variable not set");
        }
        // second, we search for a context under the current working directory
        context_root = CRUSH_CONTEXT_PATH_LOCAL;
        context_path = crush_path_join(context_root, DOTCRUSH);
        succeeded = crush_context_try_load_from_path(context_path, context);
        light_free(context_path);
        if(!succeeded) {
        // finally, we look for the default context located in user home
                context_root = light_platform_get_user_home();
                context_path = crush_path_join(context_root, DOTCRUSH);
                succeeded = crush_context_try_load_from_path(context_path, context);
                light_free(context_path);
        }
        if(!succeeded) {
                light_info("crush context not found, creating default context under user home");
                crush_context_create_under_path(light_platform_get_user_home(), context);
        }
        return context;
}

// at this stage, we test for the existence of a context by verifying that we can open a handle to
// ${context}/context.json; if the json file does not contain a well-formed crush context, this is
// treated as an error
bool crush_context_try_load_from_path(uint8_t *path, struct crush_context *context)
{
        uint8_t *real_path = realpath(path, NULL);
        uint8_t *ctx_file_path = NULL;
        //root_file_path = crush_path_join(context_path, CRUSH_CONTEXT_ROOT_JSON_FILE);
        //light_free(context_path);
        if(!real_path) {
                goto _error;
        }
        ctx_file_path = crush_path_join(real_path, CRUSH_CONTEXT_JSON_FILE);
        FILE *ctx_file = fopen(ctx_file_path, "r");
        if(!ctx_file) {
                goto _error;
        }
        json_error_t err;
        crush_json_t *context_root_json = json_loadf(ctx_file, 0, &err);
        fclose(ctx_file);
        light_free(ctx_file_path);

        // we have loaded our context root as a json object, ready to be parsed
        light_debug("found crush context root at filesystem path '%s'", real_path);
        context->path = real_path;
        context->parent = current_context;
        current_context = context;
        //light_debug("root context object: \n%s", json_dumps(context_root_json, (JSON_INDENT(8) | JSON_ENSURE_ASCII)));
        for(uint8_t i = 0; i < next_loader; i++) {
                const uint8_t *object_file;
                json_error_t err;
                if( 0 != json_unpack_ex(context_root_json, &err, 0,
                                "{s:{s:s}}", "contextObjects", loader[i].name, &object_file)) {
                        light_warn("context object loader for object type '%s' did not find matching context entry", loader[i].name);
                        light_debug("json load error: %s:%d:%d: %s", err.source, err.line, err.column, err.text);
                        continue;
                }
                const uint8_t *object_path = realpath(crush_path_join(context->path, object_file), NULL);
                // NOTE the receiving object loader becomes the owner of the json object and is responsible
                // for de-referencing it when it is no longer needed
                crush_json_t *object_json = json_load_file(object_path, (JSON_INDENT(8) | JSON_ENSURE_ASCII | JSON_DECODE_INT_AS_REAL), &err);
                if(!object_json) {
                        light_warn("context object loader for object type '%s' failed to load object storage file '%s'", loader[i].name, object_file);
                        light_debug("json load error: %s:%d:%d: %s", err.source, err.line, err.column, err.text);
                        continue;
                }
                loader[i].load(context, object_path, object_json);
        }
        
        // before return we should release the in-memory JSON object structure
        json_decref(context_root_json);
        return true;
_error:
        // failure to find a context is not strictly an error, so just return false
        light_debug("no crush context found at filesystem path '%s'", path);
        if(real_path) light_free(real_path);
        if(ctx_file_path) light_free(ctx_file_path);
        return false;
}
// the structure of the context data structure will presumably evolve to become more flexible
// in terms of the content it may contain. right now we have the four core context objects
// (i.e. module, display, font, render) statically defined in our default structure
void crush_context_create_under_path(uint8_t *path, struct crush_context *context)
{
        uint8_t *root_path = realpath(path, NULL);
        if(!root_path) {
                light_debug("could not resolve filesystem path '%s' (realpath() returned NULL)", path);
                goto _error;
        }
        uint8_t *ctx_dir_path = crush_path_join(root_path, DOTCRUSH);
        // default directory permissions are 0774
        if(mkdir(ctx_dir_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                light_debug("could not create default context directory at path '%s', mkdir() error code '0x%x'", root_path, errno);
                goto _error;
        }
        light_free(root_path);
        
        json_t *loader_set = json_object();
        for(uint8_t i = 0; i < next_loader; i++) {
                uint8_t result = json_object_set_new(loader_set, loader[i].name, json_string(loader[i].filename));
                crush_json_t *json = loader[i].create();
                uint8_t *obj_file_path = crush_path_join(ctx_dir_path, loader[i].filename);
                light_debug("writing context object store '%s'", obj_file_path);
                int obj_file_handle = creat(obj_file_path, (S_IRWXU | S_IRGRP | S_IROTH));
                json_dumpfd(json, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
                write(obj_file_handle, "\n", 1);
                close(obj_file_handle);
        }
        uint8_t *ctx_file_path = crush_path_join(ctx_dir_path, JSON_FILE);
        int ctx_file_handle = open(ctx_file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_t *context_obj = json_pack(
                "{"
                        "s:i,"                  // "version":           SCHEMA_VERSION,
                        "s:s,"                  // "type":              OBJECT_NAME,
                        "s:o"                   // "contextObjects:"    "loader_set"
                "}",
                "version", SCHEMA_VERSION, "type", OBJECT_NAME, "contextObjects", loader_set);
        json_dumpfd(context_obj, ctx_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        // because only animals don't leave a newline at the end of the file'p;[[mnnnnnnnnnnnnb ]]
        write(ctx_file_handle, "\n", 1);
        close(ctx_file_handle);
        json_decref(context_obj);
        light_free(ctx_file_path);

        light_free(ctx_dir_path);
        return;

_error:
        // if something goes wrong, die
        light_fatal("failed to create context at path '%s'", path);
}
void crush_context_add_context_object(struct crush_context *context, uint8_t *name, void *object)
{
        struct crush_context_object *cco = &context->object[context->object_count++];
        cco->name = name;
        cco->object = object;
}
void *crush_context_get_context_object(struct crush_context *context, uint8_t *name)
{
        for(uint8_t i = 0; i < context->object_count; i++) {
                if(!strcmp(context->object[i].name, name)) {
                        return context->object[i].object;
                }
        }
        return NULL;
}
// NOTE this routine assumes that all input paths are normalized, and path0 specifically is
// the path to a directory with no trailing path separator
// NOTE this routine allocates heap memory for the combined path, and the caller is 
// responsible for making sure it is freed
uint8_t *crush_path_join(const uint8_t *path0, const uint8_t *path1)
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
