#include <crush.h>
#include <crush_render_backend.h>

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

Light_Command_Option_Define(opt_crush_render_new_font, &cmd_crush_render, OPTION_RENDER_NEW_FONT_NAME, OPTION_RENDER_NEW_FONT_CODE, OPTION_RENDER_NEW_FONT_DESC);
Light_Command_Option_Define(opt_crush_render_new_display, &cmd_crush_render, OPTION_RENDER_NEW_DISPLAY_NAME, OPTION_RENDER_NEW_DISPLAY_CODE, OPTION_RENDER_NEW_DISPLAY_DESC);

static void print_usage_render();
static void print_usage_render_new();
static void print_usage_render_info();
static void print_usage_render_list();
static void callback__render_job_done(struct render_job *job, void *arg);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_RENDER_CONTEXT_OBJECT_NAME

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:i,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:i,s:[]}"

void crush_render_module_load()
{
        crush_common_register_context_object_loader(CRUSH_RENDER_CONTEXT_OBJECT_NAME, CRUSH_RENDER_CONTEXT_JSON_FILE, 
                                        crush_render_create_context, crush_render_load_context);
}
extern void crush_render_module_unload()
{
        struct crush_render_context *default_context = crush_render_context();
        struct render_engine *default_engine = render_engine_default();
        light_debug("saving all pending changes to the object database");
        crush_render_context_commit(default_context);
        light_debug("shutting down default rendering pipeline");
        render_engine_cmd_shutdown(default_engine);
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
        render_ctx->root = context;
        render_ctx->file_path = file_path;
        json_unpack(
                data,
                CONTEXT_OBJECT_FMT,
                "version", &render_ctx->version, "type", &type, "next_id", &render_ctx->next_id,
                "contextRenders", &render_ctx->data);
        json_decref(data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, CRUSH_MODULE_CONTEXT_OBJECT_NAME, render_ctx);
}
struct crush_render *crush_render_context_get(struct crush_render_context *context, uint8_t id)
{
        ID_To_String(id_str, id);
        crush_json_t *obj_data = json_object_getn(context->data, id_str, CRUSH_JSON_KEY_LENGTH);
        struct crush_render *result = crush_render_object_deserialize(obj_data);
        json_decref(obj_data);
        result->context = context;
        return result;
}
uint8_t crush_render_context_save(struct crush_render_context *context, struct crush_render *object)
{
        if(object->id == CRUSH_JSON_ID_NEW) {
                light_debug("saving new object, name: '%s'", object->name);
        } else {
                light_debug("saving object ID 0x%8X, name: '%s'", object->id, object->name);
        }
        ID_To_String(id_str, object->id);
        return json_object_setn_new(context->data, id_str, CRUSH_JSON_KEY_LENGTH, crush_render_object_serialize(object));
}
uint8_t crush_render_context_commit(struct crush_render_context *context)
{
        light_debug("writing context '%s' to disk", context->file_path);
        ID_To_String(id_str, context->next_id);
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT,
                                        "version",              context->version,
                                        "type",                 CRUSH_FONT_CONTEXT_OBJECT_NAME,
                                        "next_id",              context->next_id,
                                        "contextRenders",       context->data);
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        json_decref(obj_data);
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);

        return 0;
}

crush_json_t *crush_render_object_serialize(struct crush_render *object)
{
        json_t *data = json_pack(
                "{"
                        "s:s,"          //      "name": "render:sans_helvetica/$disp/$version"
                        "s:i,"          //      "font": "sans_helvetica"
                        "s:i,"          //      "display": "$disp"
                        "s:s"           //      "path": "data/render/sans_helvetica/$disp/$version"
                "}",
                "name",         object->name,
                "font",         crush_font_get_id(object->font),
                "display",      crush_display_get_id(object->display),
                "path",         object->path
                );
        return data;
}
struct crush_render *crush_render_object_deserialize(crush_json_t *data)
{
        uint32_t font_id, display_id;
        struct crush_render *object = light_alloc(sizeof(struct crush_render));
        json_unpack(data, 
                "{"
                        "s:s,"          //      "name":                 "crush:render:$id"
                        "s:i,"          //      "font": "sans_helvetica"
                        "s:i,"          //      "display": "$disp"
                        "s:s,"          //      "path":                 "$context/data/render/$id"
                "}",
                "name",         &object->name,
                "font",         &font_id,
                "display",       &display_id,
                "path",         &object->path
        );
        json_decref(data);
        return object;
}

void crush_render_init(struct crush_render *render, struct crush_font *font, uint8_t font_size, struct crush_display *display, const uint8_t *name)
{
        render->state = CRUSH_RENDER_STATE_NEW;
        render->id = CRUSH_JSON_LPRIME;
        render->job_id = RENDER_JOB_NEW;
        render->font = font;
        render->display = display;
        uint8_t *time_str = crush_common_datetime_string();
        char **name_char = (char **) &render->name;
        asprintf(name_char, "render:%s@%dpt-%s-%s",
                        crush_font_get_name(font), font_size, crush_display_get_name(display), time_str);
        light_free(time_str);
}
void crush_render_release(struct crush_render *render)
{
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
uint8_t crush_render_add_render_job(struct crush_render *render)
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
        uint8_t job = render_engine_create_render_job(render_engine_default(), render->name, render->font, render->font_size, render->display, callback__render_job_done, render->path);
        if(job == RENDER_JOB_ERR) {
                ID_To_String(id_str, render->id);
                light_debug("failed to queue render job '%s', object ID 0x%s", render->name, id_str);
        }
        light_info("rendering font face '%s' at %dpt for display '%s'", render->font->name, render->font_size, render->display->name);
        render->job_id = job;
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
}

static struct light_cli_invocation_result do_cmd_render(struct light_cli_invocation *invoke)
{
        print_usage_render();
        return (struct light_cli_invocation_result) {.code = LIGHT_CLI_RESULT_SUCCESS};
}
static struct light_cli_invocation_result do_cmd_render_new(struct light_cli_invocation *invoke)
{
        const uint8_t *str_font = light_cli_invocation_get_option_value(invoke, OPTION_RENDER_NEW_FONT_NAME);
        if(!str_font) {
                str_font = light_platform_getenv(CRUSH_EV_FONT);
        }
        if(!str_font) {
                light_error("command 'crush render new' called without setting either '--font' option or ${CRUSH_FONT} environment variable");
                return Result_Error;
        }
        struct crush_font *font = crush_font_get_by_name(str_font);
        if(!font) {
                light_error("could not find font object with name '%s'", str_font);
                return Result_Error;
        }
        const uint8_t *str_display = light_cli_invocation_get_option_value(invoke, OPTION_RENDER_NEW_DISPLAY_NAME);
        if(!str_display) {
                str_display = light_platform_getenv(CRUSH_EV_DISPLAY);
        }
        if(!str_display) {
                light_error("command 'crush render new' called without setting either '--display' option or ${CRUSH_DISPLAY} environment variable");
                return Result_Error;
        }
        struct crush_display *display = crush_display_get_by_name(str_display);
        if(!display) {
                light_error("could not find display object with ID '%s'", str_display);
                return Result_Error;
        }
        light_info("creating new render job '%s-%s", font->name, display->name);
        struct crush_render *new_render = light_alloc(sizeof(struct crush_render));
        new_render->font = font;
        new_render->display = display;

        struct crush_render_context *context = crush_render_context();
        uint32_t id_old, id_new;
        do {
                id_old = context->next_id;
                id_new = crush_common_get_next_counter_value(id_old);
        } while(!atomic_compare_exchange_weak(&context->next_id, &id_old, id_new));
        new_render->id = id_old;
        new_render->state = CRUSH_RENDER_STATE_NEW;
        crush_render_context_save(context, new_render);
        // this command performs the actual file write which saves our new object to disk
        crush_render_context_commit(context);

        // this command queues a new render job for processing
        crush_render_add_render_job(new_render);
        
        return Result_Success;
}
// NOTE that this callback is executed on the background worker stack
static void callback__render_job_done(struct render_job *job, void *arg)
{
        struct crush_render *render = (struct crush_render *)arg;
        atomic_store(&render->output, job->result);
        atomic_store(&render->state, CRUSH_RENDER_STATE_DONE);

        crush_render_context_save(render->context, render);
        crush_render_context_commit(render->context);

        if(job->res_pitch >= 0) {
                printf("echo render job '%s':\n");
                for(uint8_t i = 0; i < strlen(RENDER_CHAR_SET); i++) {
                        printf("\n");
                        printf(job->result[i]);
                        light_free(job->result[i]);
                        printf("\n");
                }
                light_free(job->result);
                light_free(job);
        }
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
