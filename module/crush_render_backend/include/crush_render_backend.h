#ifndef _CRUSH_RENDER_BACKEND_H
#define _CRUSH_RENDER_BACKEND_H

//   TODO the structures and API need to be refactored in terms of the coupling between threads, queues,
// and the rendering engine class itself. currently the rendering engine object is monolithic and enforces
// a 1:1 relationship between itself, one thread and one queue. it is desirable to have much more flexible
// control over the render pipeline, although it remains to be seen exactly what the ideal reworking of this
// pattern looks like.
//   TODO one aspect which immediately presents itself is the use of thread pooling as a scalable approach to
// improving performance through parallelism. this would likely resemble a list of active threads embedded
// within and managed by the engine object itself. however, this would call for facilities to automatically
// manage the set of work threads given the resources of the compute environment, which is another whole thing.

#include <stdint.h>
#include <threads.h>
#include <freetype2/ft2build.h>
#include <freetype/freetype.h>

#define ENGINE_INIT             -1
#define ENGINE_ONLINE           0
#define ENGINE_SUSPEND          1
#define ENGINE_ERROR            2

// job state codes
#define JOB_READY               0       // <- this is the state of new jobs in the queue
#define JOB_ACTIVE              1       // <- the job is running. at most one job per engine
                                        //   can be active at one time.
#define JOB_ERROR               2       // <- something has gone wrong with the execution of this job

#define RENDER_JOB_MAX          8
#define RENDER_JOB_NEW          RENDER_JOB_MAX
#define RENDER_JOB_ERR          RENDER_JOB_MAX

struct render_job {
        uint8_t state;
        struct crush_font *font;
        uint8_t font_size;
        struct crush_display *display;
        uint8_t *output_path;
};

struct render_engine {
        const uint8_t *name;
        struct render_job *active_job;
        FT_Library freetype;
        atomic_uchar engine_state;
        thrd_t work_thread;
        struct crush_queue work_queue;
};

extern void render_backend_init();
extern void render_backend_shutdown();
extern struct render_engine *render_engine_default();

extern uint8_t render_engine_init(struct render_engine *engine, const uint8_t *name, bool launch);
extern uint8_t render_engine_get_state(struct render_engine *engine);
extern uint8_t render_engine_engine_is_online(struct render_engine *engine);
extern struct render_job *render_engine_get_active_job(struct render_engine *engine);
extern uint8_t render_engine_get_job_count(struct render_engine *engine);
extern struct render_job *render_engine_get_job(struct render_engine *engine, uint8_t id);
extern uint8_t render_engine_create_render_job(struct render_engine *engine, struct crush_font *font, uint8_t font_size, struct crush_display *target_display, uint8_t *output_path);
extern void render_engine_suspend_processing(struct render_engine *engine);
extern void render_engine_resume_processing(struct render_engine *engine);
extern void render_engine_cancel_active_job(struct render_engine *engine);
extern void render_engine_shutdown(struct render_engine *engine);

#endif