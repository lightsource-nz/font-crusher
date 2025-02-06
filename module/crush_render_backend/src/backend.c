#include <crush.h>
#include <crush_render_backend.h>

#ifdef _STDC_NO_THREADS_
#error "crush rendering engine requires C11 thread support"
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

static struct render_engine engine_default;

static int worker__render_work_thread_main(void *arg);
static void worker__render_job_process(struct render_engine *engine, struct render_job *job);

void render_backend_init()
{
        light_debug("loading default render engine");
        uint8_t status;
        if(status = render_engine_init(&engine_default, "crush:render_engine_default", true)) {
                light_fatal("failed to load default rendering engine: error code '%s'", light_error_to_string(status));
        }
        light_debug("default render engine loaded successfully");
}
void render_backend_shutdown()
{

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

        if(launch) {
                thrd_create(&engine->work_thread, worker__render_work_thread_main, engine);
        }
        return LIGHT_OK;
}
uint8_t render_engine_get_engine_state(struct render_engine *engine)
{
        return engine->engine_state;
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
uint8_t render_engine_create_render_job(struct render_engine *engine, struct crush_font *font, uint8_t font_size, struct crush_display *target_display, uint8_t *output_path)
{
        if(crush_queue_full(&engine->work_queue)) {
                light_error("unable to queue render job '%s', job queue full");
                return CRUSH_ERR_QUEUE;
        }
        struct render_job *job = light_alloc(sizeof(struct render_job));
        job->font = font,
        job->font_size = font_size;
        job->display = target_display;
        job->output_path = output_path;
        crush_queue_put(&engine->work_queue, job);
}

void render_engine_suspend_processing(struct render_engine *engine)
{
        
}
void render_engine_resume_processing(struct render_engine *engine);
void render_engine_cancel_active_job(struct render_engine *engine);
void render_engine_shutdown(struct render_engine *engine);

// worker thread functions:
// WARNING these functions should only be called from within the rendering engine's worker thread
static int worker__render_work_thread_main(void *arg)
{
        struct render_engine *engine = (struct render_engine *)arg;
        while(1) {
                // this operation will block the worker thread when the queue is empty
                crush_queue_get(&engine->work_queue, &engine->active_job);
                // ASSERT(engine->active_job)
                
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
        
}