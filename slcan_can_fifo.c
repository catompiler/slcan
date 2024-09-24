#include "slcan_can_fifo.h"
#include <string.h>
#include <assert.h>



void slcan_can_fifo_init(slcan_can_fifo_t* fifo)
{
    assert(fifo != NULL);

    memset(fifo->buf, 0x0, SLCAN_CAN_FIFO_SIZE * sizeof(slcan_can_fifo_data_t));
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

size_t slcan_can_fifo_put(slcan_can_fifo_t* fifo, const slcan_can_msg_t* msg, slcan_future_t* future)
{
    assert(fifo != NULL);

    if(fifo->count < SLCAN_CAN_FIFO_SIZE){
        slcan_can_fifo_data_t* data = &fifo->buf[fifo->wptr];

        memcpy(&data->can_msg, msg, sizeof(slcan_can_msg_t));
        data->future = future;

        fifo->count ++;
        fifo->wptr ++;
        if(fifo->wptr >= SLCAN_CAN_FIFO_SIZE){
            fifo->wptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_can_fifo_get(slcan_can_fifo_t* fifo, slcan_can_msg_t* msg, slcan_future_t** future)
{
    assert(fifo != NULL);

    if(msg && fifo->count > 0){
        slcan_can_fifo_data_t* data = &fifo->buf[fifo->rptr];

        memcpy(msg, &data->can_msg, sizeof(slcan_can_msg_t));
        if(future) *future = data->future;

        fifo->count --;
        fifo->rptr ++;
        if(fifo->rptr >= SLCAN_CAN_FIFO_SIZE){
            fifo->rptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_can_fifo_peek(const slcan_can_fifo_t* fifo, slcan_can_msg_t* msg, slcan_future_t** future)
{
    assert(fifo != NULL);

    if(msg && fifo->count > 0){
        const slcan_can_fifo_data_t* data = &fifo->buf[fifo->rptr];

        memcpy(msg, &data->can_msg, sizeof(slcan_can_msg_t));
        if(future) *future = data->future;

        return 1;
    }
    return 0;
}

void slcan_can_fifo_data_readed(slcan_can_fifo_t* fifo, size_t data_size)
{
    assert(fifo != NULL);

    fifo->count -= data_size;
    fifo->rptr += data_size;
    if(fifo->rptr >= SLCAN_CAN_FIFO_SIZE){
        fifo->rptr -= SLCAN_CAN_FIFO_SIZE;
    }
}

void slcan_can_fifo_data_written(slcan_can_fifo_t* fifo, size_t data_size)
{
    assert(fifo != NULL);

    fifo->count += data_size;
    fifo->wptr += data_size;
    if(fifo->wptr >= SLCAN_CAN_FIFO_SIZE){
        fifo->wptr -= SLCAN_CAN_FIFO_SIZE;
    }
}
