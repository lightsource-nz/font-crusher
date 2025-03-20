#include <crush.h>

#include <jansson.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define COMMAND_DISPLAY_NAME                    "display"
#define COMMAND_DISPLAY_DESCRIPTION             "command used to interact with entries in the crush display database"
#define COMMAND_DISPLAY_IMPORT_NAME             "import"
#define COMMAND_DISPLAY_IMPORT_DESCRIPTION      "command used to add new display device entries to the local database"
#define COMMAND_DISPLAY_INFO_NAME               "info"
#define COMMAND_DISPLAY_INFO_DESCRIPTION        "command used to show information about a specific display device entry"
#define COMMAND_DISPLAY_LIST_NAME               "list"
#define COMMAND_DISPLAY_LIST_DESCRIPTION        "command used to list all entries in the display database"

static struct light_command *cmd_display;
static struct light_command *cmd_display_import;
static struct light_command *cmd_display_info;
static struct light_command *cmd_display_list;

static void print_usage_context();
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command);
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command);

Light_Command_Define(cmd_crush_display, &cmd_crush, COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display, 0, 0);
Light_Command_Define(cmd_crush_display_import, &cmd_crush_display, COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import, 1, 1);
Light_Command_Define(cmd_crush_display_info, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);
Light_Command_Define(cmd_crush_display_list, &cmd_crush_display, COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info, 1, 1);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_DISPLAY_CONTEXT_OBJECT_NAME
#define JSON_FILE CRUSH_DISPLAY_CONTEXT_JSON_FILE

#define CONTEXT_OBJECT_FMT "{s:f,s:s,s:f,s:O}"
#define CONTEXT_OBJECT_FMT_WRITE "{s:i,s:s,s:i,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:i,s:{}}"

uint8_t crush_display_onload()
{
        crush_common_register_context_object_loader(OBJECT_NAME, JSON_FILE,
                                        crush_display_create_context, crush_display_load_context);
        /*
        cmd_display = light_cli_register_subcommand(cmd_parent,
                COMMAND_DISPLAY_NAME, COMMAND_DISPLAY_DESCRIPTION, do_cmd_display);
        cmd_display_import = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_IMPORT_NAME, COMMAND_DISPLAY_IMPORT_DESCRIPTION, do_cmd_display_import);
        cmd_display_info = light_cli_register_subcommand(cmd_display,
                COMMAND_DISPLAY_INFO_NAME, COMMAND_DISPLAY_INFO_DESCRIPTION, do_cmd_display_info);
        */
       
        return CODE_OK;
}
struct crush_display_context *crush_display_context()
{
        return crush_display_get_context(crush_context());
}
struct crush_display_context *crush_display_get_context(struct crush_context *root)
{
        return crush_context_get_context_object_type(root, OBJECT_NAME, struct crush_display_context *);
}
crush_json_t *crush_display_create_context()
{
        json_t *display_obj = json_pack(
                CONTEXT_OBJECT_NEW_FMT,
                "version",      SCHEMA_VERSION,
                "type",         OBJECT_NAME,
                "next_id",      crush_common_get_initial_counter_value(),
                "contextDisplays");
                return display_obj;
}
void crush_display_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *json)
{
        uint8_t *type;
        struct crush_display_context *display_context = light_alloc(sizeof(struct crush_display_context));
        light_mutex_init(&display_context->lock);
        display_context->root = context;
        display_context->file_path = file_path;
        double version_f, next_id_f;
        json_unpack(json,
                CONTEXT_OBJECT_FMT,
                "version", &version_f,
                "type", &type,
                "next_id", &next_id_f,
                "contextDisplays", &display_context->data);
        if(strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        display_context->version = (uint16_t) version_f;
        display_context->next_id = (uint32_t) next_id_f;
        crush_context_add_context_object(context, OBJECT_NAME, (void *)display_context);
}
struct crush_display *crush_display_context_get(struct crush_display_context *context, uint32_t id)
{
        ID_To_String(id_str, id);
        crush_json_t *obj_data = json_object_get(context->data, id_str);
        struct crush_display *result = crush_display_object_deserialize(obj_data);
        result->id = id;
        json_decref(obj_data);
        return result;
}
struct crush_display *crush_display_context_get_by_name(struct crush_display_context *ctx, const uint8_t *name)
{
        const uint8_t *_key;
        json_t *_val;
        light_mutex_do_lock(&ctx->lock);
        // TODO place sync barriers around access to the object store
        json_object_foreach(ctx->data, _key, _val) {
                if(strcmp(json_string_value(json_object_get(_val, "name")), name)) {
                        struct crush_display *out = crush_display_object_deserialize(_val);
                        json_decref(_val);
                        return out;
                }
                json_decref(_val);
        }
        light_mutex_do_unlock(&ctx->lock);
}
uint8_t crush_display_context_save(struct crush_display_context *context, struct crush_display *object)
{
        // if crush_display_save() is called on an object with no context attached, attach
        // object to the current context
        if(!context && !object->context)
                context = object->context = crush_display_context();
        if(!context)
                context = object->context;
        if(!object->context)
                object->context = context;
        
        light_mutex_do_lock(&context->lock);
        if(object->id == CRUSH_JSON_ID_NEW) {
                light_debug("saving new object, name: '%s'", object->name);
                uint32_t id_old, id_new;
                do {
                        id_old = context->next_id;
                        id_new = crush_common_get_next_counter_value(id_old);
                } while(!atomic_compare_exchange_weak(&context->next_id, &id_old, id_new));
                object->id = id_old;
        } else {
                light_debug("saving object ID 0x%8X, name: '%s'", object->id, object->name);
        }
        ID_To_String(id_str, object->id);
        if(0 != json_object_set_new(context->data, id_str, crush_display_object_serialize(object))) {
                light_mutex_do_unlock(&context->lock);
                light_warn("save operation failed for object ID 0x%X, name: '%s'", object->id, object->name);
                return LIGHT_STORAGE;
        }
        light_mutex_do_unlock(&context->lock);
        return LIGHT_OK;
}
uint8_t crush_display_context_commit(struct crush_display_context *context)
{
        light_debug("saving context to file '%s'", context->file_path);
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT_WRITE,
                                        "version",              context->version,
                                        "type",                 OBJECT_NAME,
                                        "next_id",              context->next_id,
                                        "contextDisplays",       context->data);
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        json_decref(obj_data);
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);

        return 0;
}

crush_json_t *crush_display_object_serialize(struct crush_display *object)
{
        json_t *data = json_pack(
                "{"
                        "s:s,"          //      "name": "gme_12864_6"
                        "s:s,"          //      "description": "PM-OLED, dimensions x*y, resolution 128*64"
                        "s:i,"          //      "res_h": 128
                        "s:i"           //      "res_v": 64
                        "s:i,"          //      "ppi_h": 128
                        "s:i"           //      "ppi_v": 64
                        "s:F,"          //      "width_mm": 25
                        "s:F"           //      "height_mm": 11
                "}",
                "name",                 object->name,
                "description",          object->description,
                "res_h",                object->resolution_h,
                "res_v",                object->resolution_v,
                "ppi_h",                object->ppi_h,
                "ppi_v",                object->ppi_v,
                "width_mm",             object->width_mm,
                "height_mm",            object->height_mm
                );
        return data;
}
// NOTE this function decodes the processed object onto the stack and then releases the
// json data object, before finally allocating heap memory and copying the decoded object
// back off the stack. the rationale behind this approach is to ensure that heap memory
// is not allocated until the data record has been successfully unpacked, using the stack
// as a decoding cache, since on some (embedded) platforms it may be much more costly to
// allocate heap memory than to push a fairly small object onto the stack, and on some
// platforms the allocation of a heap object may in fact be irreversible. 
struct crush_display *crush_display_object_deserialize(crush_json_t *data)
{
        struct crush_display object;
        int failed = json_unpack(data, 
                "{"
                        "s:s,"          //      "name"
                        "s:s,"          //      "description"
                        "s:i,"          //      "res_h"
                        "s:i"           //      "res_v"
                        "s:i,"          //      "ppi_h"
                        "s:i"           //      "ppi_v"
                        "s:F,"          //      "width_mm"
                        "s:F"           //      "height_mm"
                "}",
                "name",                 &object.name,
                "description",          &object.description,
                "res_h",                &object.resolution_h,
                "res_v",                &object.resolution_v,
                "ppi_h",                &object.ppi_h,
                "ppi_v",                &object.ppi_v,
                "width_mm",             &object.width_mm,
                "height_mm",            &object.height_mm
        );
        if(failed) {
                light_error("json object decode failed: json_unpack returned nonzero value");
                return NULL;
        }
        json_decref(data);
        struct crush_display *out = light_alloc(sizeof(struct crush_display));
        memcpy(out, &object, sizeof(struct crush_display));
        return out;
}
extern void crush_display_release(struct crush_display *display)
{
        if(display->json) {
                json_decref(display->json);
        }
        light_free(display);
}

void crush_display_init(struct crush_display *display, const uint8_t *name, const uint8_t *description,
                uint16_t res_h, uint16_t res_v, uint16_t ppi_h, uint16_t ppi_v, uint8_t pixel_depth)
{
        atomic_store(&display->id, CRUSH_JSON_ID_NEW);
        display->context = NULL;
        display->name = name;
        display->description = description;
        display->resolution_h = res_h;
        display->resolution_v = res_v;
        display->ppi_h = ppi_h;
        display->ppi_v = ppi_v;
        display->pixel_depth = pixel_depth;
}
uint32_t crush_display_get_id(struct crush_display *display)
{
        return display->id;
}
const uint8_t *crush_display_get_name(struct crush_display *display)
{
        return display->name;
}
static void print_usage_context()
{
        printf(
                "Usage:\n"
                "crush display info <display_id> \n"
                "crush display import <import_path> \n"
        );
}
static struct light_cli_invocation_result do_cmd_display(struct light_cli_invocation *command)
{
        return Result_Alias(&cmd_crush_display_list);
}
static struct light_cli_invocation_result do_cmd_display_import(struct light_cli_invocation *command)
{
        // TODO write this command
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_display_info(struct light_cli_invocation *command)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_display_list(struct light_cli_invocation *command)
{
        return Result_Success;
}
