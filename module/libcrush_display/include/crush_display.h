#ifndef _CRUSH_DISPLAY_H
#define _CRUSH_DISPLAY_H

#define CRUSH_DISPLAY_CONTEXT_OBJECT_NAME       "crush:display"
#define CRUSH_DISPLAY_CONTEXT_JSON_FILE         "display.json"

Light_Command_Declare(cmd_crush_display, cmd_crush);
Light_Command_Declare(cmd_crush_display_add, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_info, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_list, cmd_crush_display);
Light_Command_Declare(cmd_crush_display_remove, cmd_crush_display);

struct crush_display {
        struct crush_display_context *context;
        crush_json_t *json;
        //   _Atomic because crush_display_context_save() reaches it with atomic_store(). C11
        // requires the operand of an atomic operation to BE an atomic object: gcc accepts a
        // plain type here, clang rejects it, and clang is right -- an atomic operation on a
        // non-atomic object is ill-formed, not merely unchecked
        _Atomic uint32_t id;
        const uint8_t *name;
        const uint8_t *description;
        uint16_t resolution_h;
        uint16_t resolution_v;
        double ppi_h;
        double ppi_v;
        uint8_t pixel_depth;
        double height_mm;
        double width_mm;
};

struct crush_display_context {
        light_mutex_t lock;
        struct crush_context *root;
        uint16_t version;
        //   `data` is the sub-object taken with an incref by the O in CONTEXT_OBJECT_FMT.
        // `data_root` is the whole document the loader handed over -- the loader becomes its
        // owner, so it has to be kept to be released, or the document leaks
        crush_json_t *data;
        crush_json_t *data_root;
        const uint8_t *file_path;
        _Atomic uint32_t next_id;
};

extern uint8_t crush_display_onload(void);
// the counterpart to crush_display_onload(), called from the module's LF_EVENT_MODULE_UNLOAD handler
extern uint8_t crush_display_onunload(void);
extern struct crush_display_context *crush_display_context(void);
extern struct crush_display_context *crush_display_get_context(struct crush_context *root);
// releases everything crush_display_load_context() built; caller detaches it first
extern void crush_display_release_context(struct crush_display_context *context);
extern crush_json_t *crush_display_create_context(void);
extern void crush_display_load_context(struct crush_context *context, const uint8_t *file_path, crush_json_t *json);
extern struct crush_display *crush_display_context_get(struct crush_display_context *context, uint32_t id);
extern struct crush_display *crush_display_context_get_by_id_string(struct crush_display_context *ctx, const uint8_t *id_string);
extern struct crush_display *crush_display_context_get_by_name(struct crush_display_context *ctx, const uint8_t *name);
static inline struct crush_display *crush_display_get(uint32_t id)
{
        return crush_display_context_get(crush_display_context(), id);
}
static inline struct crush_display *crush_display_get_by_id_string(const uint8_t *id_string)
{
        return crush_display_context_get_by_id_string(crush_display_context(), id_string);
}
static inline struct crush_display *crush_display_get_by_name(const uint8_t *name)
{
        return crush_display_context_get_by_name(crush_display_context(), name);
}
extern uint8_t crush_display_context_save(struct crush_display_context *context, struct crush_display *object);
extern uint8_t crush_display_context_refresh(struct crush_display_context *context, struct crush_display *object);
extern uint8_t crush_display_context_commit(struct crush_display_context *context);
static inline uint8_t crush_display_save(struct crush_display *object)
{
        return crush_display_context_save(object->context, object);
}
static inline uint8_t crush_display_refresh(struct crush_display *object)
{
        return crush_display_context_refresh(object->context, object);
}
static inline uint8_t crush_display_commit(void)
{
        return crush_display_context_commit(crush_display_context());
}

extern crush_json_t *crush_display_object_serialize(struct crush_display *display);
extern void crush_display_object_extract(crush_json_t *data, struct crush_display *object);
extern struct crush_display *crush_display_object_deserialize(crush_json_t *data);
extern void crush_display_release(struct crush_display *display);

extern void crush_display_init(struct crush_display *display, const uint8_t *name, const uint8_t *description,
        uint16_t res_h, uint16_t res_v, double ppi_h, double ppi_v, uint8_t pixel_depth);
extern uint32_t crush_display_get_id(struct crush_display *display);
extern const uint8_t *crush_display_get_name(struct crush_display *display);

#define crush_display_get_context_object(_context) \
        crush_context_get_context_object_type(_context, CRUSH_DISPLAY_CONTEXT_OBJECT_NAME, struct crush_display_context *)

static inline struct light_command *crush_display_get_command(void)
{
        return &cmd_crush_display;
}
static inline struct light_command *crush_display_get_subcommand_import(void)
{
        return &cmd_crush_display_add;
}
static inline struct light_command *crush_display_get_subcommand_info(void)
{
        return &cmd_crush_display_info;
}
static inline struct light_command *crush_display_get_subcommand_list(void)
{
        return &cmd_crush_display_list;
}
static inline struct light_command *crush_display_get_subcommand_remove(void)
{
        return &cmd_crush_display_remove;
}

#endif
