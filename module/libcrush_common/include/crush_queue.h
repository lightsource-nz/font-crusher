#ifndef _CRUSH_QUEUE_H
#define _CRUSH_QUEUE_H

#define QUEUE_MAX               16
#define QUEUE_NULL              QUEUE_MAX

#define QUEUE_OK                0u
#define QUEUE_FAIL              1u
// -> this structure employs a simple protocol to encode its state, where the [head] index
// is set to an out-of-range value (QUEUE_NULL) when the queue is full, and the [tail]
// index to the same value when the queue is empty. this provides a consistent and compact
// encoding of the queue's contents and its internal state.
struct crush_queue {
        mtx_t lock;
        cnd_t read_cnd;
        cnd_t write_cnd;
        atomic_uchar write_head;
        atomic_uchar read_head;
        void *cell[QUEUE_MAX];
};

#define crush_queue_get(queue, out) _crush_queue_get(queue, (void **) out)
#define crush_queue_put(queue, item) _crush_queue_put(queue, (void *) item)
#define crush_queue_get_nonblock(queue, out) _crush_queue_get_nonblock(queue, (void **) out)
#define crush_queue_put_nonblock(queue, item) _crush_queue_put_nonblock(queue, (void *) item)
extern void crush_queue_init(struct crush_queue *queue);
extern uint8_t crush_queue_count(struct crush_queue *queue);
extern void *crush_queue_peek_idx(struct crush_queue *queue, uint8_t index);
static inline void *crush_queue_peek(struct crush_queue *queue)
{
        return crush_queue_peek_idx(queue, 0);
}
extern bool crush_queue_full(struct crush_queue *queue);
extern bool crush_queue_empty(struct crush_queue *queue);
extern void _crush_queue_get(struct crush_queue *queue, void **out);
extern void _crush_queue_put(struct crush_queue *queue, void *item);
extern uint8_t _crush_queue_put_nonblock(struct crush_queue *queue, void *item);
extern uint8_t _crush_queue_get_nonblock(struct crush_queue *queue, void **out);

#endif
