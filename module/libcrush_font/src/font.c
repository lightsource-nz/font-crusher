#include <crush.h>

#include <jansson.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#define COMMAND_FONT_NAME "font"
#define COMMAND_FONT_DESCRIPTION "main subcommand for access to the crush font database (aliased to 'font list')"
#define COMMAND_FONT_ADD_NAME "add"
#define COMMAND_FONT_ADD_DESCRIPTION "adds a font to the local crush database, from a web URL or local file"
#define COMMAND_FONT_REMOVE_NAME "remove"
#define COMMAND_FONT_REMOVE_DESCRIPTION "removes a font from the local crush database"
#define COMMAND_FONT_INFO_NAME "info"
#define COMMAND_FONT_INFO_DESCRIPTION "displays information about a font in the local crush database"
#define COMMAND_FONT_LIST_NAME "list"
#define COMMAND_FONT_LIST_DESCRIPTION "displays a list of all fonts stored in the local crush database"

#define COMMAND_FONT_ADD_OPT_LOCALFILE_CODE             'l'
#define COMMAND_FONT_ADD_OPT_LOCALFILE_NAME             "--local-file"
#define COMMAND_FONT_ADD_OPT_LOCALFILE_DESC             "indicates that the argument given is a path to a local file, not a URL"
#define COMMAND_FONT_ADD_OPT_FACE_INDEX_CODE             'f'
#define COMMAND_FONT_ADD_OPT_FACE_INDEX_NAME             "--face-index"
#define COMMAND_FONT_ADD_OPT_FACE_INDEX_DESC             "indicates which typeface index should be selected from the given font file"

static void print_usage_font();
static void print_usage_font_add();
static void print_usage_font_remove();
static void print_usage_font_info();
static void print_usage_font_list();
static struct light_cli_invocation_result do_cmd_font(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_add(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_remove(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_info(struct light_cli_invocation *invoke);
static struct light_cli_invocation_result do_cmd_font_list(struct light_cli_invocation *invoke);

Light_Command_Define(cmd_crush_font, &cmd_crush, COMMAND_FONT_NAME, COMMAND_FONT_DESCRIPTION, do_cmd_font, 0, 0);
Light_Command_Define(cmd_crush_font_add, &cmd_crush_font, COMMAND_FONT_ADD_NAME, COMMAND_FONT_ADD_DESCRIPTION, do_cmd_font_add, 1, 1);
Light_Command_Define(cmd_crush_font_remove, &cmd_crush_font, COMMAND_FONT_REMOVE_NAME, COMMAND_FONT_REMOVE_DESCRIPTION, do_cmd_font_remove, 1, 1);
Light_Command_Define(cmd_crush_font_info, &cmd_crush_font, COMMAND_FONT_INFO_NAME, COMMAND_FONT_INFO_DESCRIPTION, do_cmd_font_info, 1, 1);
Light_Command_Define(cmd_crush_font_list, &cmd_crush_font, COMMAND_FONT_LIST_NAME, COMMAND_FONT_LIST_DESCRIPTION, do_cmd_font_list, 0, 0);

Light_Command_Switch_Define(cmd_crush_font_add__opt_localfile, &cmd_crush_font_add, COMMAND_FONT_ADD_OPT_LOCALFILE_NAME, COMMAND_FONT_ADD_OPT_LOCALFILE_CODE, COMMAND_FONT_ADD_OPT_LOCALFILE_DESC);
Light_Command_Switch_Define(cmd_crush_font_add__opt_face_index, &cmd_crush_font_add, COMMAND_FONT_ADD_OPT_FACE_INDEX_NAME, COMMAND_FONT_ADD_OPT_FACE_INDEX_CODE, COMMAND_FONT_ADD_OPT_FACE_INDEX_DESC);

#define SCHEMA_VERSION CRUSH_CONTEXT_JSON_SCHEMA_VERSION
#define OBJECT_NAME CRUSH_FONT_CONTEXT_OBJECT_NAME
#define JSON_FILE CRUSH_FONT_CONTEXT_JSON_FILE

#define CONTEXT_OBJECT_FMT "{s:i,s:s,s:i,s:O}"
#define CONTEXT_OBJECT_NEW_FMT "{s:i,s:s,s:i,s:[]}"

uint8_t crush_font_onload()
{
        crush_common_register_context_object_loader(OBJECT_NAME, JSON_FILE,
                                        crush_font_create_context, crush_font_load_context);
        return LIGHT_OK;
}
struct crush_font_context *crush_font_context()
{
        return crush_font_get_context(crush_context());
}
struct crush_font_context *crush_font_get_context(struct crush_context *root)
{
        return crush_context_get_context_object_type(root, OBJECT_NAME, struct crush_font_context *);
}
crush_json_t *crush_font_create_context()
{
        return json_pack(CONTEXT_OBJECT_NEW_FMT, "version", SCHEMA_VERSION, "type", OBJECT_NAME, "contextFonts");
}
void crush_font_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *data)
{
        uint8_t *type;
        struct crush_font_context *font_ctx = light_alloc(sizeof(struct crush_font_context));
        font_ctx->root = context;
        font_ctx->file_path = file_path;
        json_unpack(data, 
                CONTEXT_OBJECT_FMT,
                "version", &font_ctx->version,
                "type", &type,
                "next_id", &font_ctx->next_id,
                "contextFonts", &font_ctx->data);
        if(!strcmp(type, OBJECT_NAME)) {
                light_fatal("attempted to load object store of type '%s' (expected '%s')", type, OBJECT_NAME);
        }
        crush_context_add_context_object(context, OBJECT_NAME, font_ctx);
}

struct crush_font *crush_font_context_get(struct crush_font_context *context, const uint32_t id)
{
        ID_To_String(id_str, id);
        crush_json_t *obj_data = json_object_getn(context->data, id_str, CRUSH_JSON_KEY_LENGTH);
        struct crush_font *result = crush_font_object_deserialize(obj_data);
        json_decref(obj_data);
        return result;
}
struct crush_font *crush_font_context_get_by_name(struct crush_font_context *context, const uint8_t *name)
{
        const char *key;
        json_t *value;
        json_object_foreach(context->data, key, value) {
                if(strcmp(json_string_value(json_object_get(value, "name")), name)) {
                        struct crush_font *out = crush_font_object_deserialize(value);
                        json_decref(value);
                        return out;
                }
                json_decref(value);
        }
        return NULL;
}
uint8_t crush_font_context_save(struct crush_font_context *context, struct crush_font *object)
{
        // if crush_font_save() is called on an object with no context attached, attach
        // object to the current context
        if(!context && !object->context)
                context = object->context = crush_font_context();
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
        } else {
                light_debug("saving object ID 0x%8X, name: '%s'", object->id, object->name);
        }
        ID_To_String(id_str, object->id);
        return json_object_set_new(context->data, id_str, crush_font_object_serialize(object));
}
uint8_t crush_font_context_commit(struct crush_font_context *context)
{
        int obj_file_handle = open(context->file_path, (O_WRONLY|O_CREAT|O_TRUNC), (S_IRWXU | S_IRGRP | S_IROTH));
        json_t *obj_data = json_pack(CONTEXT_OBJECT_FMT,
                                        "version",              context->version,
                                        "type",                 OBJECT_NAME,
                                        "next_id",              context->next_id,
                                        "contextFonts", context->data);
        json_dumpfd(obj_data, obj_file_handle, (JSON_INDENT(8) | JSON_ENSURE_ASCII));
        write(obj_file_handle, "\n", 1);
        close(obj_file_handle);
        json_decref(obj_data);

        return 0;
}

crush_json_t *crush_font_object_serialize(struct crush_font *font)
{
        const uint8_t *state_str = crush_font_state_string(font->state);
        json_t *files = json_array();
        for(uint8_t i = 0; i < font->file_count; i++) {
                json_array_append_new(files, json_string(font->file[i]));
        }
        json_t *obj = json_pack(
                "{"
                        "s:s,"          //      "name":                 "font_creator.sans_helvetica"
                        "s:s,"          //      "state":                "STATE_NEW"
                        "s:b,"          //      "source_is_local"       false
                        "s:s,"          //      "source":               "git:https//github.com/font_creator/sans_helvetica"
                        "s:s,"          //      "path"                  "data/font/font_creator.sans_helvetica"
                        "s:s,"          //      "target_file"           "font_creator.sans_helvetica.ttf"
                        "s:i,"          //      "face_index"            0
                        "s:O"           //      "files":                [...]
                "}",
                "name",         font->name,
                "state",        state_str,
                "source_is_local",      font->source_is_local,
                "source",       font->source,
                "path",         font->path,
                "target_file",  font->target_file,
                "face_index",   font->face_index,
                "files",        files
                );
        return obj;
}
// ==> struct crush_font *crush_font_object_deserialize(crush_json_t data)
//   we may come back and implement full custom field extraction for objects of differing
// font_type values, but for now all font objects are expected to have the same fields
struct crush_font *crush_font_object_deserialize(crush_json_t *data)
{
        struct crush_font *font = light_alloc(sizeof(struct crush_font));
        crush_json_t *files_data;
        json_unpack(data, 
                "{"

                "s:s,"          //      "name":                 "font_creator.sans_helvetica"
                "s:s,"          //      "state":                "STATE_NEW"
                "s:b,"          //      "source_is_local"       false
                "s:s,"          //      "source":               "git:https//github.com/font_creator/sans_helvetica"
                "s:s,"          //      "path"                  "data/font/font_creator.sans_helvetica"
                "s:s,"          //      "target_file"           "font_creator.sans_helvetica.ttf"
                "s:i,"          //      "face_index"            0
                "s:O"           //      "files":                [...]
                "}",
                "name",         &font->name,
                "source_url",   &font->source,
                "font_root",    &font->path,
                "files",        &files_data
        );
        uint8_t i;
        json_t *file_value;
        json_array_foreach(files_data, i, file_value) {
                json_unpack(file_value, "s", &font->file[i]);
        }
        json_decref(data);

        return font;
}

uint32_t crush_font_get_id(struct crush_font *font)
{
        return font->id;
}
const uint8_t *crush_font_get_target_file(struct crush_font *font)
{
        return font->file[font->target_file];
}
uint8_t crush_font_get_target_face_index(struct crush_font *font)
{
        return font->face_index;
}
const uint8_t *crush_font_get_name(struct crush_font *font)
{
        return font->name;
}
bool crush_font_get_source_local(struct crush_font *font)
{
        return font->source_is_local;
}

void crush_font_init(struct crush_font *font, const uint8_t *name, const uint8_t *source_url)
{
        atomic_store(&font->state, CRUSH_FONT_STATE_NEW);
        font->id = CRUSH_JSON_ID_NEW;
        font->context = NULL;
        font->name = name;
        font->source = source_url;
        font->target_file = 0;
        font->face_index = 0;
        font->file_count = 0;
        font->source_is_local = false;
}
void crush_font_init_local(struct crush_font *font, const uint8_t *name, const uint8_t *file_path)
{
        crush_font_init(font, name, file_path);
        font->source_is_local = true;
}
void crush_font_release(struct crush_font *object)
{
        if(object->data)
                json_decref(object->data);
        light_free(object);
}
void crush_font_set_id(struct crush_font *font, uint32_t id)
{
        font->id = id;
}
void *crush_font_set_name(struct crush_font *font, const uint8_t *name)
{
        font->name = name;
}
void crush_font_set_target_file(struct crush_font *font, const uint8_t *filename)
{
        for(uint8_t i = 0; i < font->file_count; i++) {
                if(strcmp(font->file[i], filename)) {
                        font->target_file = i;
                        return;
                }
        }
}
void crush_font_set_target_face_index(struct crush_font *font, uint8_t index)
{
        // TODO it would be good to bounds-check this incoming value,
        // if we can find a sensible boundary value to check against
        font->face_index = index;
}
void crush_font_add_file(struct crush_font *font, const uint8_t *filename)
{
        if(font->file_count >= CRUSH_FONT_FILE_MAX){
                light_error("cannot add file to font object '%s': max files reached for object", font->name);
                return;
        }
        font->file[font->file_count++] = filename;
}

const uint8_t *crush_font_state_string(uint8_t state)
{
        switch(state) {
                case CRUSH_FONT_STATE_NEW:
                return CRUSH_FONT_STATE_NEW_STR;
                case CRUSH_FONT_STATE_READY:
                return CRUSH_FONT_STATE_READY_STR;
        }
}
uint8_t crush_font_state_code(const uint8_t *state_str)
{
        if(strcmp(state_str, CRUSH_FONT_STATE_NEW_STR))
                return CRUSH_FONT_STATE_NEW;
        if(strcmp(state_str, CRUSH_FONT_STATE_READY_STR))
                return CRUSH_FONT_STATE_READY;
}

// shows information about the currently selected CRUSH_FONT, if any
static struct light_cli_invocation_result do_cmd_font(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
// command definition:  crush font add <name> <source_url> [face_index] [--include-files ...]
//                      crush font add --local-file <name> <file_path> [face_index]
// notes:
// -> rather than trying to share a library instance with the rendering subsystem, we just
//   instantiate one that is owned by the command handler thread, and release it when done
static struct light_cli_invocation_result do_cmd_font_add(struct light_cli_invocation *invoke)
{
        const uint8_t *font_name;
        const uint8_t *font_file_path;
        bool ov_localfile = light_cli_invocation_get_switch_value(invoke, COMMAND_FONT_ADD_OPT_LOCALFILE_NAME);
        if(ov_localfile) {
                font_file_path = light_cli_invocation_get_arg_value(invoke, 1);
        } else {
                light_error("crush font add with remote source is not yet implemented");
                return Result_Error;
        }
        uint8_t face_index = 0;
        if(light_cli_invocation_option_is_set(invoke, COMMAND_FONT_ADD_OPT_FACE_INDEX_NAME)) {
                face_index = atoi(light_cli_invocation_get_option_value(invoke, COMMAND_FONT_ADD_OPT_FACE_INDEX_NAME));
        }
        FT_Library freetype;
        FT_Face face;
        FT_Error err = FT_Init_FreeType(&freetype);
        if(err) {
                light_error("failed to create freetype2 instance, error message: %s", FT_Error_String(err));
                return Result_Error;
        }
        err = FT_New_Face(freetype, font_file_path, face_index, &face);
        if(err) {
                light_error("failed to load font file '%s', error message: %d", font_file_path, FT_Error_String(err));
                return Result_Error;
        }
        light_info("loaded font file '%s' successfully", font_file_path);

        struct crush_font *font = light_alloc(sizeof(struct crush_font));
        crush_font_init_local(font, basename((char *)font_file_path), font_file_path);
        
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_remove(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_info(struct light_cli_invocation *invoke)
{
        if(invoke->args_bound < 1) {
                light_error("not enough args bound to run command (%d, requires 1)", invoke->args_bound);
                return Result_Error;
        }
        struct crush_font *font = crush_font_get_by_name(invoke->arg[0]);
        light_info("Crush Font record:");
        light_info("font name: '%s'", font->name);
        light_info("source URL: '%s'", font->source);
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_font_list(struct light_cli_invocation *invoke)
{
        return Result_Success;
}
static void print_usage_font()
{
        printf(
                "Usage:\n"
                "crush font [list] [<options>] \n"
                "crush font info <name> [<options>] \n"
                "crush font add <name> <source-url> [<options>] \n"
                "crush font remove <name> [<options>] \n"
        );
}

static void print_usage_font_add()
{
        printf(
                "Usage:\n"
                "crush font add <name> <path> [<options>] \n"
                "crush font add <name> <source-url> [<options>] \n"
        );
}
static void print_usage_font_remove()
{
        printf(
                "Usage:\n"
                "crush font remove <name> [<options>] \n"
        );
}
static void print_usage_font_info()
{
        printf(
                "Usage:\n"
                "crush font info <name> [<options>] \n"
        );
}
static void print_usage_font_list()
{
        printf(
                "Usage:\n"
                "crush font list [<options>] \n"
        );
}