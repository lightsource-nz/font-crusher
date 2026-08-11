#include <crush_common.h>

void crush_queue_init(struct crush_queue *queue)
{
        int status;
        status = mtx_init(&queue->lock, mtx_plain);
        mtx_lock(&queue->lock);
        cnd_init(&queue->read_cnd);
        cnd_init(&queue->write_cnd);
        queue->is_open = true;
        // we start out with the write head set to zero, and the read head set to NULL
        queue->write_head = 0;
        queue->read_head = QUEUE_NULL;
        mtx_unlock(&queue->lock);
}
void crush_queue_close(struct crush_queue *queue)
{
        // is_open is part of the predicate the waiters test under this lock, so it and the
        // broadcasts belong under it too. set from outside, a close can land between a
        // waiter's test and its wait and be missed entirely -- leaving a thread asleep on a
        // queue that will never be written to again, which is a hang at shutdown rather
        // than at startup but no less permanent
        mtx_lock(&queue->lock);
        queue->is_open = false;
        cnd_broadcast(&queue->read_cnd);
        cnd_broadcast(&queue->write_cnd);
        mtx_unlock(&queue->lock);
}
void crush_queue_deinit(struct crush_queue *queue)
{
        cnd_destroy(&queue->read_cnd);
        cnd_destroy(&queue->write_cnd);
        mtx_destroy(&queue->lock);
}
static uint8_t increment_index(uint8_t index)
{
        return (index + 1) % QUEUE_MAX;
}
uint8_t crush_queue_count(struct crush_queue *queue)
{
        if(queue->read_head == QUEUE_NULL) return 0;
        if(queue->write_head == QUEUE_NULL) return QUEUE_MAX;
        if(queue->write_head > queue->read_head) return (queue->write_head - queue->read_head);
        else return QUEUE_MAX - (queue->read_head - queue->write_head);
}
void *crush_queue_peek_idx(struct crush_queue *queue, uint8_t index)
{
        if(index >= crush_queue_count(queue)) return NULL;
        if(queue->write_head > queue->read_head)
                return queue->cell[index - (queue->write_head - queue->read_head - 1)];
        else
                return queue->cell[index - (queue->write_head - queue->read_head - 1)];
}
bool crush_queue_full(struct crush_queue *queue)
{
        return queue->write_head == QUEUE_NULL;
}
bool crush_queue_empty(struct crush_queue *queue)
{
        return queue->read_head == QUEUE_NULL;
}
//   TODO I'm pretty sure this routine has sufficient atomic protection that it should be
// safe to remove the locks, once the other routines accessing the read and write heads are
// also determined to be safe
uint8_t _crush_queue_put(struct crush_queue *queue, void *item)
{
        mtx_lock(&queue->lock);
        // is_open is part of the condition, not just fullness: crush_queue_close() broadcasts
        // write_cnd, but without this a producer blocked on a full queue would re-test only
        // write_head, find it unchanged, and go straight back to sleep on a dead queue
        while(queue->write_head == QUEUE_NULL && queue->is_open) {
                cnd_wait(&queue->write_cnd, &queue->lock);
        }
        if(!queue->is_open) {
                mtx_unlock(&queue->lock);
                return QUEUE_FAIL;
        }
        // computed AFTER the wait rather than before it: a queue that held items when this
        // call started can have been drained completely while we waited for space, and a
        // was_empty captured up front would then be stale, skip the signal below, and
        // strand a consumer already asleep on read_cnd
        uint8_t was_empty = crush_queue_empty(queue);
        uint8_t index;
        do {
                index = queue->write_head;
        }
        while(!atomic_compare_exchange_weak(&queue->write_head, &index, (index + 1) % QUEUE_MAX));
        queue->cell[index] = item;
        // read_head is the predicate _crush_queue_get() tests while holding this same lock,
        // so it and the signal have to be published under the lock as well. done outside it
        // (as this was), the signal can land in the window after a consumer has tested the
        // predicate but before it reaches cnd_wait -- the wakeup is lost, and because every
        // later put sees was_empty == 0 and skips signalling, nothing ever wakes it again.
        // that is a permanent stall with an item sitting in the queue, and it is exactly
        // what hung `crush render new` intermittently
        if(was_empty) {
                queue->read_head = index;
                cnd_signal(&queue->read_cnd);
        }
        mtx_unlock(&queue->lock);
        return QUEUE_OK;
}
uint8_t _crush_queue_get(struct crush_queue *queue, void **out)
{
        light_trace("[enter] queue: 0x%x", queue);
        if(!queue->is_open) return QUEUE_FAIL;
        mtx_lock(&queue->lock);
        // a while loop, not an if. cnd_wait may return spuriously -- C11 permits it and
        // winpthreads does it -- and the old `if` fell straight through on a spurious wake
        // with read_head still QUEUE_NULL, indexing cell[QUEUE_MAX] out of bounds. re-testing
        // the predicate also handles the queue being closed while we were asleep
        while(crush_queue_empty(queue) && queue->is_open) {
                cnd_wait(&queue->read_cnd, &queue->lock);
        }
        // still empty is only reachable if the loop above exited on !is_open. a queue closed
        // with items still in it drains them, matching what this did before
        if(crush_queue_empty(queue)) {
                mtx_unlock(&queue->lock);
                light_trace("[fail: closed] queue: 0x%x", queue);
                return QUEUE_FAIL;
        }

        uint8_t was_full = crush_queue_full(queue);
        uint8_t last_element = (crush_queue_count(queue) == 1);
        uint8_t index;
        uint8_t head_value = last_element? QUEUE_NULL : ((queue->read_head + 1) % QUEUE_MAX);
        do {
                index = queue->read_head;
        }
        while(!atomic_compare_exchange_weak(&queue->read_head, &index, head_value));
        *out = queue->cell[index];
        if(last_element) {
                queue->read_head = QUEUE_NULL;
        }
        // published under the lock for the same reason as read_head in _crush_queue_put():
        // write_head is the predicate a blocked producer re-tests, so signalling it from
        // outside the lock can lose the wakeup and stall a producer against a queue that
        // has since made room
        if(was_full) {
                queue->write_head = index;
                cnd_signal(&queue->write_cnd);
        }
        mtx_unlock(&queue->lock);
        light_trace("[exit] queue: 0x%x", queue);
        return QUEUE_OK;
}
uint8_t _crush_queue_put_nonblock(struct crush_queue *queue, void *item)
{
        mtx_lock(&queue->lock);
        uint8_t was_empty = crush_queue_empty(queue);
        if(queue->write_head == QUEUE_NULL) {
                mtx_unlock(&queue->lock);
                return QUEUE_FAIL;
        }
        uint8_t index;
        do {
                index = queue->write_head;
        }
        while(!atomic_compare_exchange_weak(&queue->write_head, &index, (index + 1) % QUEUE_MAX));

        queue->cell[index] = item;
        // under the lock, same as the blocking twin -- this variant never waits, but the
        // consumer it is waking may well be blocked in _crush_queue_get(), so losing this
        // signal stalls exactly as badly
        if(was_empty) {
                queue->read_head = index;
                cnd_signal(&queue->read_cnd);
        }
        mtx_unlock(&queue->lock);
        return QUEUE_OK;
}
uint8_t _crush_queue_get_nonblock(struct crush_queue *queue, void **out)
{
        mtx_lock(&queue->lock);
        if(queue->read_head == QUEUE_NULL) {
                mtx_unlock(&queue->lock);
                return QUEUE_FAIL;
        }
        uint8_t was_full = crush_queue_full(queue);
        uint8_t last_element = (crush_queue_count(queue) == 1);
        uint8_t index;
        uint8_t head_value = last_element? QUEUE_NULL : ((queue->read_head + 1) % QUEUE_MAX);
        do {
                index = queue->read_head;
        }
        while(!atomic_compare_exchange_weak(&queue->read_head, &index, head_value));

        *out = queue->cell[index];
        // under the lock, same as the blocking twin -- the producer being woken may be
        // blocked in _crush_queue_put()
        if(was_full) {
                queue->write_head = index;
                cnd_signal(&queue->write_cnd);
        }
        mtx_unlock(&queue->lock);
        return QUEUE_OK;
}
