#define _GNU_SOURCE
#include <crush.h>
#include <crush_render_backend.h>
#include <freetype/ftoutln.h>

#ifdef _STDC_NO_THREADS_
#error "crush rendering engine requires C11 thread support"
#endif

#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define SIGNAL_SUSPEND          SIGUSR1
#define SIGNAL_SHUTDOWN         SIGUSR2

#define WORKER_OK               0
#define WORKER_ERR              1

static struct render_engine engine_default;

static int worker__render_work_thread_main(void *arg);
static void worker__render_job_process(struct render_engine *engine, struct render_job *job);
static void worker__render_job_complete(struct render_engine *engine, struct render_job *job);
static uint8_t *worker__render_job_copy_bitmap(FT_Bitmap bitmap);
static void worker__render_dump_glyph(struct render_job *job);
static void signal__worker__render_engine_signal_handler(int signo);

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

        DIR *outdir = opendir(job->output_path);
        if(!outdir) {
                if(errno == ENOENT) {
                        mkdir(output_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
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
                pthread_kill(engine->work_thread, SIGNAL_SUSPEND);
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
                pthread_kill(engine->work_thread, SIGNAL_SUSPEND);
        } else {
                light_warn("resume failed: engine state changed unexpectedly");
        }
}
void render_engine_cmd_shutdown(struct render_engine *engine)
{
        light_debug("shutting down worker for render engine '%s'", engine->name);
        pthread_kill(engine->work_thread, SIGNAL_SHUTDOWN);
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
        pthread_kill(engine->work_thread, SIGNAL_SHUTDOWN);
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
        struct sigaction sigact_suspend;
        sigact_suspend.sa_handler = signal__worker__render_engine_signal_handler;
        sigaction(SIGNAL_SUSPEND, &sigact_suspend, NULL);
        struct sigaction sigact_shutdown;
        sigact_shutdown.sa_handler = signal__worker__render_engine_signal_handler;
        sigaction(SIGNAL_SHUTDOWN, &sigact_shutdown, NULL);
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
        FT_Error err = FT_New_Face(engine->freetype, job->font->file[job->font->target_file], job->font->face_index, &face);
        err = FT_Set_Char_Size(face, 0, job->font_size, job->display->ppi_h, job->display->ppi_v);
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
                        job->callback(job, job->cb_arg);
                        return;
                }

                // -> this will be set again by each successive glyph dumped by this render job,
                //    but the value will be correct
                job->res_pitch = face->glyph->bitmap.pitch;
                job->result[i] = worker__render_job_copy_bitmap(face->glyph->bitmap);
                FT_Done_Face(face);
        }
        light_debug("rendering complete for job '%s'", job->name);
        atomic_store(&job->state, JOB_DONE);
        job->callback(job, job->cb_arg);
}
static void worker__render_job_complete(struct render_engine *engine, struct render_job *job)
{

}
static uint8_t *worker__render_job_copy_bitmap(FT_Bitmap bitmap)
{
        uint8_t output_length = (bitmap.rows + 1) * bitmap.width + 1;
        uint8_t *output_buffer = light_alloc(output_length);
        output_buffer[output_length] = '\0';
        for(uint8_t i = 0; i < bitmap.rows; i++) {
                // use pitch to ensure correct buffer alignment when width is not a multiple of 8
                uint8_t *source_row = bitmap.buffer + i * abs(bitmap.pitch);
                for(uint8_t j = 0; j < (bitmap.width / 8); j++) {
                        uint8_t source_byte = source_row[j];
                        bool last_byte = (j + 1 == bitmap.width / 8);
                        uint8_t bits = bitmap.width % 8;
                        if(bits == 0) continue;
                        (source_byte & 0x80)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 1) continue;
                        (source_byte & 0x40)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 2) continue;
                        (source_byte & 0x20)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 3) continue;
                        (source_byte & 0x10)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 4) continue;
                        (source_byte & 0x08)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 5) continue;
                        (source_byte & 0x04)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 6) continue;
                        (source_byte & 0x02)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                        if(bits == 7) continue;
                        (source_byte & 0x01)? strcat(output_buffer, "*") : strcat(output_buffer, " ");
                }
                strcat(output_buffer, "\n");
        }
        return output_buffer;
}
// -> signal handler, handled on worker thread stack
// -> SIGNAL_SUSPEND (SIGUSR1) is defined as a suspend signal, so we pause any work in progress and halt workers.
//    if SIGNAL_SUSPEND is received again, we reset the suspend flag and resume processing
// -> SIGNAL_SHUTDOWN (SIGUSR2) is defined as a shutdown signal, so we close the input side of the queue and process
// all remaining jobs before halting
static void signal__worker__render_engine_signal_handler(int signo)
{
        uint8_t flag_val;
        uint8_t engine_state = atomic_load(&this_engine->engine_state_private);
        switch (signo)
        {
        case SIGNAL_SUSPEND:
                flag_val = false;
                if(atomic_compare_exchange_strong(&this_engine->flag_suspend, &flag_val, true)) {
                        // code in this section will run exactly once when the engine suspend state becomes true
                } else {
                        // code here runs only if the suspend flag was already set
                        atomic_store(&this_engine->flag_suspend, false);
                        return;
                }
                break;
        
        case SIGNAL_SHUTDOWN:
                flag_val = false;
                if(atomic_compare_exchange_strong(&this_engine->flag_closed, &flag_val, true)) {
                        crush_queue_close(&this_engine->work_queue);
                }
                break;
        }
}
