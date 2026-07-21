#define _GNU_SOURCE
#include <crush.h>
#include <crush_render_backend.h>
#include <freetype/ftoutln.h>

#ifdef _STDC_NO_THREADS_
#error "crush rendering engine requires C11 thread support"
#endif

#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define WORKER_OK               0
#define WORKER_ERR              1

static struct render_engine engine_default;

static int worker__render_work_thread_main(void *arg);
static void worker__render_job_process(struct render_engine *engine, struct render_job *job);
static void worker__render_job_complete(struct render_engine *engine, struct render_job *job);
static uint8_t *worker__render_job_copy_bitmap(FT_Bitmap bitmap, uint8_t cell_width, uint8_t cell_height);
static void worker__render_export_c_source(struct render_job *job);

void render_backend_init()
{
        light_debug("loading default render engine...");
        uint8_t status;
        if(status = render_engine_init(&engine_default, "crush:render_engine_default", true)) {
                light_fatal("failed to load default rendering engine: error code '%s'", light_error_to_string(status));
        }
        // TODO make an option to control the behaviour of waiting here for the engine to come online
        render_engine_engine_wait_for_online(&engine_default);
        light_debug("default render engine loaded successfully");
}
void render_backend_shutdown()
{
        light_debug("shutting down default render engine...");
        render_engine_cmd_shutdown(&engine_default);
        light_debug("done shutting down default render engine");
}

extern struct render_engine *render_engine_default()
{
        return &engine_default;
}
uint8_t render_engine_init(struct render_engine *engine, const uint8_t *name, bool launch)
{
        engine->engine_state = ENGINE_INIT;
        engine->name = name;
        light_mutex_init_recursive(&engine->lock);
        cnd_init(&engine->cond_online);
        if(launch) {
                int res = thrd_create(&engine->work_thread, worker__render_work_thread_main, (void *)engine);
                if(res != thrd_success) {
                        light_error("failed to create render engine worker thread: thrd_create returned value 0x%x", res);
                        return LIGHT_EXTERNAL;
                }
        }
        return LIGHT_OK;
}
uint8_t render_engine_init_launch(struct render_engine *engine, const uint8_t *name)
{
        render_engine_init(engine, name, true);
        light_mutex_do_lock(&engine->lock);
        if(render_engine_get_engine_state(engine) != ENGINE_ONLINE) {
                light_condition_wait(&engine->cond_online, &engine->lock);
        }
        light_mutex_do_unlock(&engine->lock);
}
uint8_t render_engine_get_engine_state(struct render_engine *engine)
{
        return atomic_load(&engine->engine_state);
}
const uint8_t *render_engine_get_name(struct render_engine *engine)
{
        return engine->name;
}
uint8_t render_engine_engine_is_online(struct render_engine *engine)
{
        return atomic_load(&engine->engine_state) == ENGINE_ONLINE;
}
void render_engine_engine_wait_for_online(struct render_engine *engine)
{
        if(engine->engine_state != ENGINE_INIT)
                return;

        light_mutex_t mutex;
        light_mutex_init_recursive(&mutex);
        light_mutex_do_lock(&mutex);
        cnd_wait(&engine->cond_online, &mutex);
        light_mutex_do_unlock(&mutex);
}
struct render_job *render_engine_get_active_job(struct render_engine *engine)
{
        return engine->active_job;
}
uint8_t render_engine_get_job_count(struct render_engine *engine)
{
        return crush_queue_count(&engine->work_queue);
}
struct render_job *render_engine_get_job(struct render_engine *engine, uint8_t id)
{
        if(id >= 0 && id < RENDER_JOB_MAX) {
                return crush_queue_peek_idx(&engine->work_queue, id);
        }
        return NULL;
}

static int8_t render_job_file_indexof(const uint8_t **files, const uint8_t *name)
{
        for(uint8_t i = 0; i<  i < CRUSH_FONT_FILE_MAX; i++) {
                if(strcmp(name, files[i]))
                        return i;
        }
        return UINT8_MAX;
}
// output_path is typically two levels below the context root (e.g. ".crush/render/<job-name>"),
// and the intermediate "render" directory doesn't exist in a fresh context, so a single mkdir()
// (which, unlike "mkdir -p", requires the parent to already exist) silently fails with ENOENT
static void _mkdir_recursive(const uint8_t *path)
{
        uint8_t buf[CRUSH_MAX_PATH_LENGTH];
        strncpy(buf, path, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        for(uint8_t *p = buf + 1; *p; p++) {
                if(*p == '/') {
                        *p = '\0';
                        mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                        *p = '/';
                }
        }
        mkdir(buf, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
struct render_job *render_engine_create_render_job(struct render_engine *engine, const uint8_t *name, struct crush_font *font, uint8_t font_size, struct crush_display *target_display, void (*callback)(struct render_job *, void *), void *cb_arg, uint8_t *output_path)
{
        atomic_bool closed = atomic_load(&engine->flag_closed);
        if(closed) {
                light_warn("failed to queue new render job '%s': render engine '%s' already shutting down");
                return NULL;
        }
        struct render_job *job = light_alloc(sizeof(struct render_job));
        if(!job) {
                light_warn("failed to queue new render job '%s': out of memory", name);
                return NULL;
        }
        job->caller = thrd_current();
        job->callback = callback;
        job->cb_arg = cb_arg;
        job->name = name;
        job->font = font,
        job->font_size = font_size;
        job->display = target_display;
        job->output_path = output_path;
        job->progress = 0;
        job->prog_max = sizeof(RENDER_CHAR_SET);
        job->state = JOB_READY;

        DIR *outdir = opendir(job->output_path);
        if(!outdir) {
                if(errno == ENOENT) {
                        _mkdir_recursive(output_path);
                }
        } else {
                closedir(outdir);
        }
        uint8_t err = crush_queue_put(&engine->work_queue, job);
        if(err) {
                light_debug("failed to queue render job '%s'", job->name);
                light_free(job);
                return NULL;
        }
        light_debug("render job '%s' queued successfully at system time %d", job->name, light_platform_get_time_since_init());
        return job;
}
struct render_job *render_engine_collect_render_job(struct render_engine *engine)
{
        struct render_job *job;
        crush_queue_get(&engine->result_queue, &job);
        return job;
}
struct render_job *render_engine_try_collect_render_job(struct render_engine *engine)
{
        struct render_job *job;
        if(crush_queue_get_nonblock(&engine->result_queue, &job))
                return NULL;
        return job;
}
void render_engine_cmd_set_mode(struct render_engine *engine, uint8_t mode)
{
        atomic_store(&engine->queue_mode, mode);
}

// command: launch
// lock-free: TRUE
void render_engine_cmd_launch(struct render_engine *engine)
{
        uint8_t engine_state = atomic_load(&engine->engine_state);
        if(engine_state != ENGINE_INIT) {
                light_warn("attempted to launch '%s' when not in INIT state", render_engine_get_name(engine));
                return;
        }
        // if the CX operation fails, the engine was no longer in INIT state, so no point in re-trying
        if(atomic_compare_exchange_strong(&engine->engine_state, &engine_state, ENGINE_ONLINE)) {
                light_debug("launching worker thread for render engine '%s'", render_engine_get_name(engine));
                thrd_create(&engine->work_thread, worker__render_work_thread_main, engine);
                char *thread_name;
                asprintf(&thread_name, "render_engine_worker__%s", render_engine_get_name(engine));
                pthread_setname_np(engine->work_thread, thread_name);
        } else {
                light_warn("launch failed: engine state changed unexpectedly");
        }
}
void render_engine_cmd_suspend_processing(struct render_engine *engine)
{
        uint8_t engine_state = atomic_load(&engine->engine_state);
        if(engine_state != ENGINE_ONLINE) {
                light_warn("attempted to suspend '%s' when not in ONLINE state", render_engine_get_name(engine));
                return;
        }
        // if the CX operation fails, the engine is no longer in ONLINE state, so no point in re-trying
        if(atomic_compare_exchange_strong(&engine->engine_state, &engine_state, ENGINE_SUSPEND)) {
                light_debug("suspending render engine '%s'", render_engine_get_name(engine));
                atomic_store(&engine->flag_suspend, true);
        } else {
                light_warn("suspend failed: engine state changed unexpectedly");
        }
}
void render_engine_cmd_resume_processing(struct render_engine *engine)
{
        uint8_t engine_state = engine->engine_state;
        if(engine_state != ENGINE_SUSPEND) {
                light_warn("attempted to resume render engine '%s' when not in SUSPEND state", render_engine_get_name(engine));
                return;
        }
        // if the CX operation fails, the engine is no longer in SUSPEND state, so no point in re-trying
        if(atomic_compare_exchange_strong(&engine->engine_state, &engine_state, ENGINE_ONLINE)) {
                light_debug("resuming render engine worker '%s'", render_engine_get_name(engine));
                atomic_store(&engine->flag_suspend, false);
        } else {
                light_warn("resume failed: engine state changed unexpectedly");
        }
}
static void _render_engine_signal_shutdown(struct render_engine *engine)
{
        atomic_bool flag_val = false;
        if(atomic_compare_exchange_strong(&engine->flag_closed, &flag_val, true)) {
                crush_queue_close(&engine->work_queue);
        }
}
void render_engine_cmd_shutdown(struct render_engine *engine)
{
        light_debug("shutting down worker for render engine '%s'", engine->name);
        _render_engine_signal_shutdown(engine);
        int exit_code;
        thrd_join(engine->work_thread, &exit_code);
        if(exit_code) {
                light_debug("engine worker (for '%s') terminated with exit code", engine->name);
        } else {
                light_debug("engine worker (for '%s') terminated successfully", engine->name);
        }
}
void render_engine_cmd_shutdown_async(struct render_engine *engine)
{
        light_debug("shutting down worker for render engine '%s'", engine->name);
        _render_engine_signal_shutdown(engine);
}
void render_engine_cmd_wait_for_shutdown(struct render_engine *engine)
{
        light_debug("waiting for engine worker for '%s' to terminate", engine->name);
        int exit_code;
        thrd_join(engine->work_thread, &exit_code);
        if(exit_code) {
                light_debug("engine worker (for '%s') terminated with exit code", engine->name);
        } else {
                light_debug("engine worker (for '%s') terminated successfully", engine->name);
        }
}

#define STATE_READ              0
#define STATE_PROCESS           1
// worker thread functions:
// WARNING these functions should only be called from within the rendering engine's worker thread
static thread_local struct render_engine *this_engine;
static void worker__render_engine_event_handle(struct render_engine *engine);
static int worker__render_work_thread_main(void *arg)
{
        this_engine = (struct render_engine *)arg;
        light_debug("loading worker thread for render engine '%s'", this_engine->name);
        atomic_store(&this_engine->engine_state, ENGINE_INIT);
        atomic_thread_fence(memory_order_release);
        int err;
        if(err = FT_Init_FreeType(&this_engine->freetype)) {
                light_error("failed to initialise the freetype2 typesetting library: FT_Init_FreeType() returned value %d", err);
                return LIGHT_EXTERNAL;
        }
        int major, minor, patch;
        FT_Library_Version(this_engine->freetype, &major, &minor, &patch);
        light_debug("loaded freetype2 version %d.%d.%d", major, minor, patch);

        crush_queue_init(&this_engine->work_queue);
        crush_queue_init(&this_engine->result_queue);
        atomic_store(&this_engine->engine_state, ENGINE_ONLINE);
        atomic_thread_fence(memory_order_release);
        cnd_broadcast(&this_engine->cond_online);
        while(1) {
                atomic_store(&this_engine->engine_state_private, STATE_READ);
                atomic_signal_fence(memory_order_release);
                worker__render_engine_event_handle(this_engine);
                // check in case the queue was closed while the worker was sleeping
                if(this_engine->flag_closed && crush_queue_empty(&this_engine->work_queue))
                        break;
                // this operation will block the worker thread when the queue is empty,
                // but will immediately return QUEUE_FAIL if the queue is closed
                if(QUEUE_OK != crush_queue_get(&this_engine->work_queue, &this_engine->active_job)) {
                        break;
                }
                atomic_store(&this_engine->engine_state_private, STATE_PROCESS);
                atomic_signal_fence(memory_order_release);
                worker__render_job_process(this_engine, this_engine->active_job);
                crush_queue_put(&this_engine->result_queue, this_engine->active_job);
                this_engine->active_job = NULL;
                if(this_engine->flag_closed && crush_queue_empty(&this_engine->work_queue)) {
                        atomic_store(&this_engine->engine_state, ENGINE_HALT);
                        break;
                }
        }
        // at this point, the queue is closed and empty (i.e. all pending events have been processed)
        // the worker thread just needs to release all resources it is holding and then terminate
        light_debug("worker thread for render engine '%s' terminating...", this_engine->name);
        FT_Done_FreeType(this_engine->freetype);
        return LIGHT_OK;
}
// the length of this interval is somewhat arbitrary since the purpose is just to suspend
// the thread until it receives a signal from an external source
#define WORKER_SLEEP_INTERVAL_SEC       20
static void worker__render_engine_event_handle(struct render_engine *engine)
{
        struct timespec interval = { .tv_sec = WORKER_SLEEP_INTERVAL_SEC };
        atomic_bool suspend;
        while(suspend = atomic_load(&engine->flag_suspend)){
                thrd_sleep(&interval, NULL);
        }
}
static void worker__render_job_process(struct render_engine *engine, struct render_job *job)
{

        light_debug("running render job '%s'", job->name);
        FT_Face face = NULL;
        // assert (job->state == JOB_READY)
        if(job->state != JOB_READY) {
                light_error("render job not ready: %s", job->name);
        }
        job->progress = UINT16_MAX;
        job->state = JOB_ACTIVE;
        // font->file[] holds bare filenames (see crush_font_object_extract()), so the path
        // must be joined with font->path (the font's storage directory) before use
        uint8_t *font_file_path = crush_path_join(job->font->path, job->font->file[job->font->target_file]);
        FT_Error err = FT_New_Face(engine->freetype, font_file_path, job->font->face_index, &face);
        light_free(font_file_path);
        if(err) {
                light_error("job '%s' rendering failed: FT_New_Face() returned code 0x%x: %s", job->name, err, FT_Error_String(err));
                job->state = JOB_ERROR;
                job->callback(job, job->cb_arg);
                return;
        }
        // FT_Set_Char_Size() takes sizes in 26.6 fixed-point (1/64th of a point), not whole points
        err = FT_Set_Char_Size(face, 0, job->font_size * 64, job->display->ppi_h, job->display->ppi_v);
        // the glyph cell size is a fixed input to the rest of this pipeline, derived once from
        // the font's own nominal metrics -- NOT computed by scanning actual rendered glyphs --
        // so every glyph below is rasterized directly into this same size (26.6 fixed-point,
        // hence the >> 6)
        job->cell_width = face->size->metrics.max_advance >> 6;
        job->cell_height = face->size->metrics.height >> 6;
        job->res_pitch = (job->cell_width + 7) / 8;
        uint8_t *char_list = RENDER_CHAR_SET;
        uint8_t num_glyphs = strlen(char_list);
        job->result = calloc(sizeof(void *), num_glyphs);
        int32_t load_flags = FT_LOAD_RENDER;
        if(job->display->pixel_depth == 1) {
                load_flags |= FT_LOAD_MONOCHROME;
        }
        for(uint8_t i = 0; i < num_glyphs; i++) {
                atomic_signal_fence(memory_order_release);
                atomic_store(&job->progress, i);
                // this may block the worker thread according to external events
                worker__render_engine_event_handle(engine);
                // err = FT_Load_Glyph(face, char_list[i], load_flags);
                // FT_Outline_Translate(face->glyph->outline, face->glyph->metrics.)
                err = FT_Load_Char(face, char_list[i], load_flags);
                if(err) {
                        light_error("job '%s' rendering failed: FT_Load_Char() returned code 0x%x: %s", job->name, err, FT_Error_String(err));
                        job->state = JOB_ERROR;
                        FT_Done_Face(face);
                        job->callback(job, job->cb_arg);
                        return;
                }
                job->result[i] = worker__render_job_copy_bitmap(face->glyph->bitmap, job->cell_width, job->cell_height);
        }
        FT_Done_Face(face);
        worker__render_export_c_source(job);
        light_debug("rendering complete for job '%s'", job->name);
        atomic_store(&job->state, JOB_DONE);
        job->callback(job, job->cb_arg);
}
static void worker__render_job_complete(struct render_engine *engine, struct render_job *job)
{

}
static bool _bitmap_get_pixel(const uint8_t *buffer, int pitch, uint8_t x, uint8_t y)
{
        uint8_t byte = buffer[y * abs(pitch) + x / 8];
        return (byte >> (7 - (x % 8))) & 1;
}
static void _bitmap_set_pixel(uint8_t *buffer, uint8_t pitch, uint8_t x, uint8_t y)
{
        buffer[y * pitch + x / 8] |= (1 << (7 - (x % 8)));
}
// copies 'bitmap' into a freshly-allocated, zeroed buffer of exactly (cell_width+7)/8 *
// cell_height bytes (1 bit per pixel, MSB-first, row-major), anchored at the top-left corner.
// 'bitmap' is clipped if it's larger than the target cell in either dimension -- every glyph
// must come out the same size, since cell_width/cell_height are a fixed pipeline input (see
// worker__render_job_process()), not something derived per-glyph
static uint8_t *worker__render_job_copy_bitmap(FT_Bitmap bitmap, uint8_t cell_width, uint8_t cell_height)
{
        uint8_t dest_pitch = (cell_width + 7) / 8;
        uint16_t output_length = (uint16_t)dest_pitch * cell_height;
        uint8_t *output_buffer = light_alloc(output_length);
        memset(output_buffer, 0, output_length);
        uint8_t copy_rows = (bitmap.rows < cell_height) ? bitmap.rows : cell_height;
        uint8_t copy_cols = (bitmap.width < cell_width) ? bitmap.width : cell_width;
        for(uint8_t y = 0; y < copy_rows; y++) {
                for(uint8_t x = 0; x < copy_cols; x++) {
                        if(_bitmap_get_pixel(bitmap.buffer, bitmap.pitch, x, y)) {
                                _bitmap_set_pixel(output_buffer, dest_pitch, x, y);
                        }
                }
        }
        return output_buffer;
}
// turns a name (e.g. a font's filename) into a valid, unique-enough C identifier fragment:
// non-alphanumeric characters become '_', and a leading digit gets a '_' prefix
static uint8_t *_sanitize_c_identifier(const uint8_t *name)
{
        size_t len = strlen(name);
        uint8_t *out = light_alloc(len + 2);
        size_t out_i = 0;
        if(len > 0 && isdigit(name[0])) {
                out[out_i++] = '_';
        }
        for(size_t i = 0; i < len; i++) {
                out[out_i++] = isalnum(name[i]) ? name[i] : '_';
        }
        out[out_i] = '\0';
        return out;
}
// RENDER_CHAR_SET is printable ASCII only, so a 128-entry table indexed directly by character
// code covers every possible entry with room to spare
#define FONT_GLYPH_TABLE_SIZE 128
// writes the render job's glyph data as a single font.h/font.c pair under job->output_path,
// suitable for embedding directly in firmware: font.c defines one packed-bitmap byte array per
// rendered glyph plus a table of pointers indexed by ASCII code, and font.h declares the
// font struct type and the extern instance that ties it all together
static void worker__render_export_c_source(struct render_job *job)
{
        uint8_t *char_list = RENDER_CHAR_SET;
        uint8_t num_glyphs = strlen(char_list);
        uint8_t dest_pitch = (job->cell_width + 7) / 8;
        uint16_t glyph_size = (uint16_t)dest_pitch * job->cell_height;

        uint8_t *ident = _sanitize_c_identifier(job->font->name);
        uint8_t *h_path = crush_path_join(job->output_path, "font.h");
        uint8_t *c_path = crush_path_join(job->output_path, "font.c");
        FILE *h = fopen(h_path, "wb");
        FILE *c = fopen(c_path, "wb");
        if(!h || !c) {
                light_warn("job '%s': failed to open font.h/font.c for writing under '%s'", job->name, job->output_path);
                if(h) fclose(h);
                if(c) fclose(c);
                light_free(h_path);
                light_free(c_path);
                light_free(ident);
                return;
        }

        fprintf(h,
                "#ifndef %s_FONT_H\n"
                "#define %s_FONT_H\n"
                "\n"
                "#include <stdint.h>\n"
                "\n"
                "// glyphs[] is indexed directly by ASCII character code; unrendered code points\n"
                "// are NULL. every non-NULL entry points to a (char_width+7)/8 * char_height byte\n"
                "// buffer: 1 bit per pixel, MSB-first, row-major, 1 = black and 0 = white\n"
                "#define %s_GLYPH_TABLE_SIZE %d\n"
                "\n"
                "struct %s_font {\n"
                "        const uint8_t *const *glyphs;\n"
                "        uint8_t char_width;\n"
                "        uint8_t char_height;\n"
                "};\n"
                "\n"
                "extern const struct %s_font %s_font;\n"
                "\n"
                "#endif\n",
                ident, ident, ident, FONT_GLYPH_TABLE_SIZE, ident, ident, ident);

        fprintf(c, "#include \"font.h\"\n\n");
        for(uint8_t i = 0; i < num_glyphs; i++) {
                if(!job->result[i]) continue;
                fprintf(c, "static const uint8_t glyph_0x%02x[] = { // '%c'", char_list[i], char_list[i]);
                for(uint16_t b = 0; b < glyph_size; b++) {
                        fprintf(c, "%s0x%02x,", (b % 12 == 0) ? "\n        " : " ", job->result[i][b]);
                }
                fprintf(c, "\n};\n\n");
        }
        fprintf(c, "static const uint8_t *const glyph_table[%s_GLYPH_TABLE_SIZE] = {\n", ident);
        for(uint8_t i = 0; i < num_glyphs; i++) {
                if(!job->result[i]) continue;
                fprintf(c, "        [0x%02x] = glyph_0x%02x,\n", char_list[i], char_list[i]);
        }
        fprintf(c, "};\n\n");
        fprintf(c,
                "const struct %s_font %s_font = {\n"
                "        .glyphs = glyph_table,\n"
                "        .char_width = %u,\n"
                "        .char_height = %u,\n"
                "};\n",
                ident, ident, job->cell_width, job->cell_height);

        fclose(h);
        fclose(c);
        light_free(h_path);
        light_free(c_path);
        light_free(ident);
}
