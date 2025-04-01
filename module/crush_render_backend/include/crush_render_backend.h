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
#define ENGINE_HALT             2
#define ENGINE_ERROR            3

// job state codes
#define JOB_READY               0       // <- the state of new jobs in the queue
#define JOB_ACTIVE              1       // <- the job is running. at most one job per engine
                                        //   can be active at one time.
#define JOB_PAUSED              2       // <- the job has been partially completed and suspended
#define JOB_DONE                3       // <- the job has been completed
#define JOB_ERROR               4       // <- something has gone wrong with the execution of this job

#define RENDER_JOB_MAX          8
#define RENDER_JOB_NEW          RENDER_JOB_MAX
#define RENDER_JOB_ERR          RENDER_JOB_MAX

#define RENDER_CHAR_SET         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz`1234567890-=~!@#$%^&*()_+[]\\{}|;':\",./<>?"

#define QUEUE_BLOCKING          1

struct render_job {
        uint8_t state;
        thrd_t caller;
        int cb_signal;
        const uint8_t *name;
        struct crush_font *font;
        uint8_t font_size;
        struct crush_display *display;
        void *cb_arg;
        void (*callback)(struct render_job *, void *);
        uint8_t *output_path;
        uint16_t progress;
        uint8_t res_pitch;
        uint8_t **result;
};

struct render_engine {
        atomic_bool flag_blocking;
        atomic_bool flag_closed;
        atomic_bool flag_suspend;
        const uint8_t *name;
        struct render_job *active_job;
        FT_Library freetype;
        atomic_uchar queue_mode;
        atomic_uchar engine_state;
        atomic_uchar engine_state_private;
        thrd_t work_thread;
        struct crush_queue work_queue;
        struct crush_queue result_queue;
};

extern void render_backend_init();
extern void render_backend_shutdown();
extern struct render_engine *render_engine_default();

extern uint8_t render_engine_init(struct render_engine *engine, const uint8_t *name, bool launch);
extern uint8_t render_engine_get_state(struct render_engine *engine);
extern const uint8_t *render_engine_get_name(struct render_engine *engine);
extern uint8_t render_engine_engine_is_online(struct render_engine *engine);
extern struct render_job *render_engine_get_active_job(struct render_engine *engine);
extern uint8_t render_engine_get_job_count(struct render_engine *engine);
extern struct render_job *render_engine_get_job(struct render_engine *engine, uint8_t id);
extern uint8_t render_engine_create_render_job(struct render_engine *engine, const uint8_t *name, struct crush_font *font, uint8_t font_size, struct crush_display *target_display, void (*callback)(struct render_job *, void *), uint8_t *output_path);
// this will block the calling thread until a job becomes available
extern struct render_job *render_engine_collect_render_job(struct render_engine *engine);
// this variant is nonblocking, and returns null if no jobs are waiting
extern struct render_job *render_engine_try_collect_render_job(struct render_engine *engine);
extern void render_engine_cmd_set_mode(struct render_engine *engine, uint8_t mode);
extern void render_engine_cmd_launch(struct render_engine *engine);
extern void render_engine_cmd_suspend_processing(struct render_engine *engine);
extern void render_engine_cmd_resume_processing(struct render_engine *engine);
extern void render_engine_cmd_shutdown(struct render_engine *engine);
extern void render_engine_cmd_shutdown_async(struct render_engine *engine);

#endif