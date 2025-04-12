#include <crush.h>
#include <crush_render_backend.h>

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <jansson.h>

#include "render_private.h"


#define COMMAND_RENDER_NAME                     "render"
#define COMMAND_RENDER_DESC                     "command used to access text rendering functions, and the data produced by them"
#define COMMAND_RENDER_NEW_NAME                 "new"
#define COMMAND_RENDER_NEW_DESC                 "command used to render a new set of character glyphs from a font file and screen information"
#define COMMAND_RENDER_INFO_NAME                "info"
#define COMMAND_RENDER_INFO_DESC                "command used to view information about a given render data object"
#define COMMAND_RENDER_LIST_NAME                "list"
#define COMMAND_RENDER_LIST_DESC                "command used to view information about a given render data object"

#define OPTION_RENDER_NEW_FONT_CODE             't'     // 't' -> typeface
#define OPTION_RENDER_NEW_FONT_NAME             "font"
#define OPTION_RENDER_NEW_FONT_DESC             "the name of the font to be rendered"
#define OPTION_RENDER_NEW_DISPLAY_CODE          'd'
#define OPTION_RENDER_NEW_DISPLAY_NAME          "display"
#define OPTION_RENDER_NEW_DISPLAY_DESC          "the name of the display the render should be optimised for"
static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_render, &cmd_crush, COMMAND_RENDER_NAME, COMMAND_RENDER_DESC, do_cmd_render, 0, 0);
Light_Command_Define(cmd_crush_render_new, &cmd_crush_render, COMMAND_RENDER_NEW_NAME, COMMAND_RENDER_NEW_DESC, do_cmd_render_new, 2, 3);
Light_Command_Define(cmd_crush_render_info, &cmd_crush_render, COMMAND_RENDER_INFO_NAME, COMMAND_RENDER_INFO_DESC, do_cmd_render_info, 0, 1);
Light_Command_Define(cmd_crush_render_list, &cmd_crush_render, COMMAND_RENDER_LIST_NAME, COMMAND_RENDER_LIST_DESC, do_cmd_render_list, 1, 2);

Light_Command_Option_Define(opt_crush_render_new_font, &cmd_crush_render_new, OPTION_RENDER_NEW_FONT_NAME, OPTION_RENDER_NEW_FONT_CODE, OPTION_RENDER_NEW_FONT_DESC);
Light_Command_Option_Define(opt_crush_render_new_display, &cmd_crush_render_new, OPTION_RENDER_NEW_DISPLAY_NAME, OPTION_RENDER_NEW_DISPLAY_CODE, OPTION_RENDER_NEW_DISPLAY_DESC);

static void print_usage_render();
static void print_usage_render_new();
static void print_usage_render_info();
static void print_usage_render_list();
static void callback__render_job_done(struct render_job *job, void *arg);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_RENDER_CONTEXT_OBJECT_NAME

#define CONTEXT_OBJECT_FMT "{s:f,s:s,s:f,s:O}"
#define CONTEXT_OBJECT_FMT_WRITE "{s:i,s:s,s:i,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:i,s:[]}"

void crush_render_module_load()
{
        crush_common_register_context_object_loader(CRUSH_RENDER_CONTEXT_OBJECT_NAME, CRUSH_RENDER_CONTEXT_JSON_FILE, 
                                        crush_render_create_context, crush_render_load_context);
}
extern void crush_render_module_unload()
{
        light_debug("saving all pending changes to the object database");
        crush_render_commit();
        // TODO consider some mechanism to unregister object stores from central context
        crush_render_destroy_context(crush_render_context());
}
struct crush_render_context *crush_render_context()
{
        return crush_render_get_context(crush_context());
}
struct crush_render_context *crush_render_get_context(struct crush_context *root)
{
        return crush_context_get_context_object_type(root, OBJECT_NAME, struct crush_render_context *);
}
crush_json_t *crush_render_create_context(uint8_t *path)
{
        uint32_t next_id = crush_common_get_initial_counter_value();
        json_t *render_obj = json_pack(
                CONTEXT_OBJECT_NEW_FMT,
                "version", SCHEMA_VERSION,
                "type", OBJECT_NAME,
                "next_id", next_id,
                "contextRenders");

        return render_obj;
}
void crush_render_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_render_context *render_ctx = light_alloc(sizeof(struct crush_render_context));
        light_mutex_init(&render_ctx->lock);
        render_ctx->root = context;
        render_ctx->file_path = file_path;
        double version_f, next_id_f;
        json_unpack(data, CONTEXT_OBJECT_FMT,
                "version",      &version_f,
                "type",         &type,
                "next_id",      &next_id_f,
                "contextRenders", &render_ctx->data);
        if(strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        render_ctx->version = (uint16_t) version_f;
        render_ctx->next_id = (uint32_t) next_id_f;
        crush_context_add_context_object(context, CRUSH_RENDER_CONTEXT_OBJECT_NAME, render_ctx);
}
void crush_render_destroy_context(struct crush_render_context *context)
{
        mtx_lock(&context->lock);
        json_decref(context->data);
        context->data = NULL;
        mtx_unlock(&context->lock);
}
struct crush_render *crush_render_context_get(struct crush_render_context *context, const uint32_t id)
{
        ID_To_String(id_str, id);
        crush_json_t *obj_data = json_object_getn(context->data, id_str, CRUSH_JSON_KEY_LENGTH);
        struct crush_render *result = crush_render_object_deserialize(obj_data);
        json_decref(obj_data);
        result->context = context;
        return result;
}
struct crush_render *crush_render_context_get_by_name(struct crush_render_context *context, const uint8_t *name)
{
        const uint8_t *_key;
        json_t *_val;
        // TODO place sync barriers around access to the object store
        json_object_foreach(context->data, _key, _val) {
                if(strcmp(json_string_value(json_object_get(_val, "name")), name)) {
                        struct crush_render *out = crush_render_object_deserialize(_val);
                        json_decref(_val);
                        return out;
                }
                json_decref(_val);
        }
}
uint8_t crush_render_context_save(struct crush_render_context *context, struct crush_render *object)
{
        // it is an error to save an object to a context if it is already bound to another
        if(context && object->context && (context != object->context)) {
                light_error("tried to save object '%s' to context '%x' when it is already bound to context '%x'", object->name, context, object->context);
                return LIGHT_INVALID;
        }
        // if crush_display_save() is called on an object with no context attached, attach
        // object to the current context
        if(!context && !object->context)
                context = object->context = crush_render_context();
        if(!context)
                context = object->context;
        if(!object->context)
                object->context = context;
        if(object->id == CRUSH_JSON_ID_NEW) {
                light_debug("saving new object, name: '%s'", object->name);
                uint32_t id_old, id_new;
                do {
                        id_old = context->next_id;
                        id_new = crush_common_get_next_counter_value(id_old);
                } while(!atomic_compare_exchange_weak(&context->next_id, &id_old, id_new));
                object->id = id_old;
        }
        
        ID_To_String(id_str, object->id);
        light_debug("saving object ID %s (raw: 0x%X), name: '%s'", id_str, object->id, object->name);
        object->data = crush_render_object_serialize(object);

        if(0 != json_object_setn_new(context->data, id_str, CRUSH_JSON_KEY_LENGTH, object->data)) {
                light_error("failed to save object ID 0x%8X, name: '%s'", object->id, object->name);
                light_mutex_do_unlock(&context->lock);
                return LIGHT_STORAGE;
        }
        light_mutex_do_unlock(&context->lock);
        return LIGHT_OK;
}
uint8_t crush_render_context_refresh(struct crush_render_context *context, struct crush_render *object)
{
        // it is an error to refresh an object from a context if it is already bound to another
        if(context && object->context && (context != object->context)) {
                light_error("tried to refresh object '%s' from context '%x' when it is already bound to context '%x'", object->name, context, object->context);
                return LIGHT_INVALID;
        }
        // if crush_render_refresh() is called on an object with no context attached, attach
        // object to the current context
        if(!context && !object->context)
                context = object->context = crush_render_context();
        if(!context)
                context = object->context;
        if(!object->context)
                object->context = context;
        /*
        ID_To_String(id_str, object->id);
        json_t *new_data = json_object_getn(context->data, id_str, CRUSH_JSON_KEY_LENGTH);
        if(!new_data) {
                light_warn("failed to refresh render object '%s': object id 0x%X not found in database");
                return LIGHT_INVALID;
        }
        json_decref(object->data);
        crush_render_object_extract(new_data, object);
        return LIGHT_OK;
        */
       crush_render_object_extract(object->data, object);
       return LIGHT_OK;
}
uint8_t crush_render_context_commit(struct crush_render_context *context)
{
        light_mutex_do_lock(&context->lock);
        light_debug("writing context '%s' to disk", context->file_path);
        ID_To_String(id_str, context->next_id);
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT_WRITE,
                                        "version",              context->version,
                                        "type",                 OBJECT_NAME,
                                        "next_id",              context->next_id,
                                        "contextRenders",       context->data);
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        json_decref(obj_data);
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);
        light_mutex_do_unlock(&context->lock);
        return 0;
}

crush_json_t *crush_render_object_serialize(struct crush_render *object)
{
        ID_To_String(font_str, crush_font_get_id(object->font));
        ID_To_String(display_str, crush_display_get_id(object->display));
        json_t *data = json_pack(
                "{"
                        "s:s,"          //      "name":                 "crush:render:$id"
                        "s:i,"          //      "state"                 CRUSH_RENDER_STATE_DONE
                        "s:s,"          //      "font":                 "sans_helvetica"
                        "s:i,"          //      "font_size"             "14"
                        "s:s,"          //      "display":              "$disp"
                        "s:s,"          //      "path":                 "$context/data/render/$id"
                "}",
                "name",         object->name,
                "state",        object->state,
                "font",         font_str,
                "font_size",    object->font_size,
                "display",      display_str,
                "path",         object->path
                );
        return data;
}
void crush_render_object_extract(crush_json_t *data, struct crush_render *object)
{
        uint8_t *font_str, *display_str;
        json_unpack(data, 
                "{"
                        "s:s,"          //      "name":                 "crush:render:$id"
                        "s:i,"          //      "state"                 CRUSH_RENDER_STATE_DONE
                        "s:s,"          //      "font":                 "sans_helvetica"
                        "s:i,"          //      "font_size"             "14"
                        "s:s,"          //      "display":              "$disp"
                        "s:s,"          //      "path":                 "$context/data/render/$id"
                "}",
                "name",         &object->name,
                "state",        &object->state,
                "font",         &font_str,
                "font_size",    &object->font_size,
                "display",      &display_str,
                "path",         &object->path
        );
        object->data = data;

        // if the font-ID or display-ID has not changed, just do a refresh on the existing object.
        // this avoids hitting the font and/or display database main indexes unneccessarily
        if(String_To_ID(font_str) == object->font->id) {
                crush_font_refresh(object->font);
        } else {
                object->font = crush_font_get_by_id_string(font_str);
        }
        if(String_To_ID(display_str) == object->display->id) {
                crush_display_refresh(object->display);
        } else {
                object->display = crush_display_get_by_id_string(display_str);
        }
}
struct crush_render *crush_render_object_deserialize(crush_json_t *data)
{
        uint8_t *font_str, *display_str;
        struct crush_render *object = light_alloc(sizeof(struct crush_render));
        crush_render_object_extract(data, object);
        return object;
}
uint8_t *crush_render_context_get_root_path(struct crush_render_context *context)
{
        crush_path_join(crush_context_get_context_root_path(context->root), CRUSH_RENDER_CONTEXT_SUBDIR_NAME);
}
void crush_render_init_ctx(struct crush_render_context *context, struct crush_render *render, const uint8_t *name, struct crush_font *font, uint8_t font_size, struct crush_display *display)
{
        render->context = context;

        atomic_store(&render->id, CRUSH_JSON_ID_NEW);
        render->render_job = NULL;
        render->state = CRUSH_RENDER_STATE_NEW;
        render->font = font;
        render->font_size = font_size;
        render->display = display;
        uint8_t *time_str = crush_common_datetime_string();
        char **name_char = (char **) &render->name;
        asprintf(name_char, "render:%s@%dpt_%s_%s",
                        crush_font_get_name(font), font_size, crush_display_get_name(display), time_str);
        light_free(time_str);
        render->path = crush_path_join(crush_render_context_get_root_path(context), name);
}
void crush_render_init(struct crush_render *render, const uint8_t *name, struct crush_font *font, uint8_t font_size, struct crush_display *display)
{
        crush_render_init_ctx(crush_render_context(), render, name, font, font_size, display);
}
void crush_render_release(struct crush_render *render)
{
        if (render->data)
                json_decref(render->data);
        light_free(render->name);
        light_free(render);
}
uint32_t crush_render_get_id(struct crush_render *render)
{
        return render->id;
}
uint8_t crush_render_get_state(struct crush_render *render)
{
        return render->state;
}
uint8_t *crush_render_get_name(struct crush_render *render)
{
        return render->name;
}
struct crush_font *crush_render_get_font(struct crush_render *render)
{
        return render->font;
}
void crush_render_set_font(struct crush_render *render, struct crush_font *font)
{
        render->font = font;
}
struct crush_display *crush_render_get_display(struct crush_render *render)
{
        return render->display;
}
void crush_render_set_display(struct crush_render *render, struct crush_display *display)
{
        render->display = display;
}
uint8_t crush_render_get_font_size(struct crush_render *render)
{
        return render->font_size;
}
void crush_render_set_font_size(struct crush_render *render, uint8_t font_size)
{
        render->font_size = font_size;
}
// -> default behaviour for state changing actions is to bail out if the
// action collides with another state change
uint8_t crush_render_create_render_job(struct crush_render *render)
{
        uint8_t state = render->state;
        if(state != CRUSH_RENDER_STATE_NEW) {
                light_error("failed to queue render item '%s': object in invalid state '0x%x'", render->name, render->state);
                return LIGHT_STATE_INVALID;
        }
        if(!atomic_compare_exchange_strong(&render->state, &state, CRUSH_RENDER_STATE_RUNNING)) {
                // TODO create some kind of global analytics to count the rate of events like state
                // collisions, cache misses etc. maybe a simple macro like this:
                // Light_Analytic_State_Collision();
                light_error("failed to queue render item '%s': state changed unexpectedly", render->state);
                return LIGHT_STATE_INVALID;
        }
        struct render_job *job = render_engine_create_render_job(render_engine_default(), render->name, render->font, render->font_size, render->display, callback__render_job_done, render, render->path);
        if(!job) {
                ID_To_String(id_str, render->id);
                light_debug("failed to queue render job '%s', object ID 0x%s", render->name, id_str);
                atomic_store(&render->state, CRUSH_RENDER_STATE_ERROR);
                crush_render_save(render);
                return LIGHT_EXTERNAL;
        }
        render->render_job = job;
        atomic_store(&render->state, CRUSH_RENDER_STATE_RUNNING);
        crush_render_save(render);
        light_info("rendering font face '%s' at %dpt for display '%s'", render->font->name, render->font_size, render->display->name);
        return LIGHT_OK;
}
uint8_t crush_render_cancel_render_job(struct crush_render *render)
{
        light_info("render job '%s' canceled", crush_render_get_name(render));
        uint8_t state = render->state;
        switch (state)
        {
        // in practice, render items are unlikely to stay in the NEW state for long enough to still be
        // in that state when a CANCEL command arrives
        case CRUSH_RENDER_STATE_NEW:
                if(!atomic_compare_exchange_strong(&render->state, &state, CRUSH_RENDER_STATE_CANCEL)) {
                        light_error("failed: state change collision");
                        return LIGHT_STATE_INVALID;
                }
                break;
        case CRUSH_RENDER_STATE_CANCEL:
                light_warn("object already in CANCEL state");
                return LIGHT_STATE_INVALID;
        case CRUSH_RENDER_STATE_PAUSE:
                
        }
        render->state = CRUSH_RENDER_STATE_CANCEL;

        crush_render_save(render);
        return LIGHT_OK;
}
uint8_t crush_render_complete_render_job(struct crush_render *render)
{
        light_debug("processing completed render job '%s'", crush_render_get_name(render));
        atomic_store(&render->output, render->render_job->result);
        // in production, by the time this code is run, the render engine should already have written
        // out the final glyph bitmaps to disk files, so this function will just log the completed
        // job and update the state of the metadata

        if(render->render_job->res_pitch >= 0) {
                light_info("echo render job '%s':\n", render->render_job->name);
                for(uint8_t i = 0; i < strlen(RENDER_CHAR_SET); i++) {
                        light_info("");
                        light_info(render->render_job->result[i]);
                        light_free(render->render_job->result[i]);
                        light_info("");
                }
                light_free(render->render_job->result);
                light_free(render->render_job);
        }
        // TODO either make an option to enable notify signals, or just remove this entirely.
        // the primary crush application use case does not require notification, as the foreground
        // thread simply polls the render job until its state changes
        //
        // pthread_kill(job->caller, CRUSH_RENDER_CALLBACK_SIGNAL);
        atomic_store(&render->state, CRUSH_RENDER_STATE_DONE);
        crush_render_save(render);
}
uint8_t crush_render_fail_render_job(struct crush_render *render)
{
        light_debug("processing completed failed job '%s'", crush_render_get_name(render));
        light_free(render->render_job);
        atomic_store(&render->state, CRUSH_RENDER_STATE_ERROR);
        crush_render_save(render);
}
static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke)
{
        print_usage_render();
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
#define POLLING_INTERVAL_MS     200
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke)
{
        const uint8_t *name = light_cli_invocation_get_arg_value(invoke, 0);
        uint8_t font_size = atoi(light_cli_invocation_get_arg_value(invoke, 1));
        const uint8_t *str_font = light_cli_invocation_get_option_value(invoke, OPTION_RENDER_NEW_FONT_NAME);
        if(!str_font) {
                str_font = light_platform_getenv(CRUSH_EV_FONT);
        }
        if(!str_font) {
                light_error("command 'crush render new' called without setting either '--font' option or ${CRUSH_FONT} environment variable");
                return Result_Error;
        }
        struct crush_font *font = crush_font_get_by_name((uint8_t *)str_font);
        if(!font) {
                light_error("could not find font object with name '%s'", str_font);
                return Result_Error;
        }
        const uint8_t *str_display = light_cli_invocation_get_option_value(invoke, OPTION_RENDER_NEW_DISPLAY_NAME);
        if(!str_display) {
                str_display = light_platform_getenv(CRUSH_EV_DISPLAY);
                if(str_display) light_debug("${CRUSH_DISPLAY} is set to '%s'", str_display);
        }
        if(!str_display) {
                light_error("command 'crush render new' called without setting either '--display' option or ${CRUSH_DISPLAY} environment variable");
                return Result_Error;
        }
        struct crush_display *display = crush_display_get_by_name(str_display);

        if(!display) {
                light_error("could not find display object with name '%s'", str_display);
                return Result_Error;
        }
        struct crush_render_context *context = crush_render_context();
        light_info("creating new render job '%s-%s", font->name, display->name);
        struct crush_render *new_render = light_alloc(sizeof(struct crush_render));
        crush_render_init(new_render, name, font, font_size, display);

        crush_render_context_save(context, new_render);
        // this command performs the actual file write which saves our new object to disk
        crush_render_context_commit(context);

        // this command queues a new render job for processing
        crush_render_create_render_job(new_render);

        // TODO develop terminal-aware output streams, and use them to display a
        // progress bar reflecting the status of the render job. set the sleep duration
        // in the loop to tune the update frequency of the status bar
        do {
                light_debug("polling render job '%s', progress: '%d%%'...", new_render->name, (new_render->render_job->progress / new_render->render_job->prog_max));
                light_platform_sleep_ms(POLLING_INTERVAL_MS);
                crush_render_refresh(new_render);
        } while(crush_render_get_state(new_render) == CRUSH_RENDER_STATE_RUNNING);
        
        return Result_Success;
}
// NOTE that this callback is executed on the background worker stack
static void callback__render_job_done(struct render_job *job, void *arg)
{
        struct crush_render *render = (struct crush_render *)arg;
        switch (job->state)
        {
        case JOB_DONE:
                crush_render_complete_render_job(render);
                break;
        case JOB_ERROR:
                crush_render_fail_render_job(render);
                break;
        default:
                light_error("invalid job state: 0x%x", job->state);
                break;
        }
        crush_render_commit();
}
static struct light_cli_invocation_result do_cmd_render_info(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_render_list(struct light_cli_invocation *invoke)
{
        return Result_Success;
}

static void print_usage_render()
{
        printf(
                "Usage:\n"
                "crush render list \n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
static void print_usage_render_new()
{
        printf(
                "Usage:\n"
                "crush render new <render_id> -f <font_id> -d <display_id> \n"
        );
}
static void print_usage_render_info()
{
        printf(
                "Usage:\n"
                "crush render info <render_id> \n"
        );
}
static void print_usage_render_list()
{
        printf(
                "Usage:\n"
                "crush render list \n"
        );
}
