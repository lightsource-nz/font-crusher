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
        queue->is_open = false;
        cnd_broadcast(&queue->read_cnd);
        cnd_broadcast(&queue->write_cnd);
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
        uint8_t was_empty = crush_queue_empty(queue);
        while(queue->write_head == QUEUE_NULL) {
                cnd_wait(&queue->write_cnd, &queue->lock);
        }
        uint8_t index;
        do {
                index = queue->write_head;
        }
        while(!atomic_compare_exchange_weak(&queue->write_head, &index, (index + 1) % QUEUE_MAX));
        queue->cell[index] = item;
        mtx_unlock(&queue->lock);
        if(was_empty) {
                queue->read_head = index;
                cnd_signal(&queue->read_cnd);
        }
}
uint8_t _crush_queue_get(struct crush_queue *queue, void **out)
{
        light_trace("[enter] queue: 0x%x", queue);
        if(!queue->is_open) return QUEUE_FAIL;
        mtx_lock(&queue->lock);
        if(queue->read_head == QUEUE_NULL)
        {
                cnd_wait(&queue->read_cnd, &queue->lock);
                if(!queue->is_open) {
                        mtx_unlock(&queue->lock);
                        light_trace("[fail: closed] queue: 0x%x", queue);
                        return QUEUE_FAIL;
                }
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
        mtx_unlock(&queue->lock);
        if(was_full) {
                queue->write_head = index;
                cnd_signal(&queue->write_cnd);
        }
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
        mtx_unlock(&queue->lock);
        if(was_empty) {
                queue->read_head = index;
                cnd_signal(&queue->read_cnd);
        }
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
        mtx_unlock(&queue->lock);
        if(was_full) {
                queue->write_head = index;
                cnd_signal(&queue->write_cnd);
        }
        return QUEUE_OK;
}
