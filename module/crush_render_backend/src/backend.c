#define _GNU_SOURCE
#include <crush.h>
#include <crush_render_backend.h>
#include <freetype/ftoutln.h>

#ifdef _STDC_NO_THREADS_
#error "crush rendering engine requires C11 thread support"
#endif

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
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
static void signal__worker__render_job_signal_handler(int signo);
// signal handler to tell the foreground thread that a render job has completed
static void signal__render_engine_signal_handler(int signo);

void render_backend_init()
{
        light_debug("loading default render engine...");
        uint8_t status;
        if(status = render_engine_init(&engine_default, "crush:render_engine_default", true)) {
                light_fatal("failed to load default rendering engine: error code '%s'", light_error_to_string(status));
        }
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
        int err;
        if(err = FT_Init_FreeType(&engine->freetype)) {
                light_error("failed to initialise the freetype2 typesetting library: FT_Init_FreeType() returned value %d", err);
                return LIGHT_EXTERNAL;
        }
        int major, minor, patch;
        FT_Library_Version(engine->freetype, &major, &minor, &patch);
        light_debug("loaded freetype2 version %d.%d.%d", major, minor, patch);

        crush_queue_init(&engine->work_queue);
        crush_queue_init(&engine->result_queue);

        if(launch) {
                thrd_create(&engine->work_thread, worker__render_work_thread_main, engine);
        }
        return LIGHT_OK;
}
uint8_t render_engine_get_engine_state(struct render_engine *engine)
{
        return engine->engine_state;
}
const uint8_t *render_engine_get_name(struct render_engine *engine)
{
        return engine->name;
}
uint8_t render_engine_engine_is_online(struct render_engine *engine)
{
        return !engine->engine_state;
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
        if(id > 0 && id < RENDER_JOB_MAX) {
                return crush_queue_peek_idx(&engine->work_queue, id);
        }
        return NULL;
}
uint8_t render_engine_create_render_job(struct render_engine *engine, const uint8_t *name, struct crush_font *font, uint8_t font_size, struct crush_display *target_display, void (*callback)(struct render_job *, void *), uint8_t *output_path)
{
        struct render_job *job = light_alloc(sizeof(struct render_job));
        job->caller = thrd_current();
        job->name = name;
        job->font = font,
        job->font_size = font_size;
        job->display = target_display;
        job->output_path = output_path;
        uint8_t queue_mode = atomic_load(&engine->queue_mode);
        if(queue_mode == QUEUE_BLOCKING) {
                crush_queue_put(&engine->work_queue, job);
        } else {
                uint8_t err = crush_queue_put_nonblock(&engine->work_queue, job);
                if(err) {
                        light_debug("failed to queue render job '%s'", job->name);
                        return LIGHT_NO_RESOURCE;
                }
        }
        return LIGHT_OK;
}
uint8_t **render_engine_collect_render_job(struct render_engine *engine)
{
        struct render_job *job;
        crush_queue_get(&engine->result_queue, &job);
        return job->result;
}
uint8_t **render_engine_try_collect_render_job(struct render_engine *engine)
{
        struct render_job *job;
        if(crush_queue_get_nonblock(&engine->result_queue, &job))
                return NULL;
        return job->result;
}
static void signal__render_engine_signal_handler(int signo)
{

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
        } else {
                light_warn("launch failed: engine state changed unexpectedly");
        }
}
void render_engine_cmd_suspend_processing(struct render_engine *engine)
{
        // TODO most of this should happen inside the signal handler, on the worker stack
        uint8_t engine_state = engine->engine_state;
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
void render_engine_cmd_resume_processing(struct render_engine *engine);
void render_engine_cmd_cancel_active_job(struct render_engine *engine);
void render_engine_cmd_shutdown(struct render_engine *engine)
{
        light_debug("shutting down worker for render engine '%s'", engine->name);
        pthread_kill(engine->work_thread, SIGNAL_SHUTDOWN);
}
#define STATE_READ              0
#define STATE_PROCESS           1
// worker thread functions:
// WARNING these functions should only be called from within the rendering engine's worker thread
static thread_local struct render_engine *this_engine;
static int worker__render_work_thread_main(void *arg)
{
        this_engine = (struct render_engine *)arg;
        this_engine->engine_state = ENGINE_ONLINE;
        atomic_signal_fence(memory_order_release);
        struct sigaction sigact;
        sigact.sa_handler = signal__worker__render_job_signal_handler;
        sigaction(SIGNAL_SUSPEND, &sigact, NULL);
        while(1) {
                atomic_store(&this_engine->engine_state_private, STATE_READ);
                atomic_signal_fence(memory_order_release);
                struct render_job *next_job;
                while(!crush_queue_get_nonblock(&this_engine->result_queue, &next_job)) {
                        atomic_store(&this_engine->engine_state_private, STATE_PROCESS);
                        next_job->callback(next_job, next_job->cb_arg);
                        atomic_store(&this_engine->engine_state_private, STATE_READ);
                }
                // this operation will block the worker thread when the queue is empty
                crush_queue_get(&this_engine->work_queue, &this_engine->active_job);
                // ASSERT(this_engine->active_job)
                atomic_store(&this_engine->engine_state_private, STATE_PROCESS);
                atomic_signal_fence(memory_order_release);
                worker__render_job_process(this_engine, this_engine->active_job);
        }
}
static void worker__render_job_process(struct render_engine *engine, struct render_job *job)
{
        // assert (job->state == JOB_READY)
        if(job->state != JOB_READY) {
                light_error("render job not ready: %s", job->output_path);
        }
        job->state = JOB_ACTIVE;
        FT_Face face;
        FT_Error err = FT_New_Face(engine->freetype, job->font->file[0], 0, &face);
        err = FT_Set_Char_Size(face, 0, job->font_size, job->display->ppi_h, job->display->ppi_v);
        uint8_t *char_list = RENDER_CHAR_SET;
        uint8_t num_glyphs = strlen(char_list);
        job->result = calloc(sizeof(void *), num_glyphs);
        int32_t load_flags = FT_LOAD_RENDER;
        if(job->display->pixel_depth == 1) {
                load_flags |= FT_LOAD_MONOCHROME;
        }
        for(uint8_t i = 0; i < num_glyphs; i++) {
                // err = FT_Load_Glyph(face, char_list[i], load_flags);
                // FT_Outline_Translate(face->glyph->outline, face->glyph->metrics.)
                err = FT_Load_Char(face, char_list[i], load_flags);
                // -> this will be set again by each successive glyph dumped by this render job,
                //    but the value will be correct
                job->res_pitch = face->glyph->bitmap.pitch;
                job->result[i] = worker__render_job_copy_bitmap(face->glyph->bitmap);
        }
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
// -> SIGUSR1 is defined as a suspend signal, so we cancel any work in progress and halt workers
// -> SIGUSR2 is defined as a shutdown signal, so we close the input side of the queue and process
// all remaining jobs before halting
static void signal__worker__render_job_signal_handler(int signo)
{
        uint8_t engine_state = atomic_load(&this_engine->engine_state_private);
        switch (signo)
        {
        case SIGNAL_SUSPEND:
                switch (engine_state)
                {
                // STATE_READ: thread is blocked and queue is empty, so we can safely exit
                case STATE_READ:
                        
                        thrd_exit(WORKER_OK);
                case STATE_PROCESS:
                        break;
                }
                break;
        
        case SIGNAL_SHUTDOWN:
                break;
        }
}
