#include "slcan_can_fifo.h"
#include <string.h>



void slcan_can_fifo_init(slcan_can_fifo_t* fifo)
{
    memset(fifo->buf, 0x0, SLCAN_CAN_FIFO_SIZE * sizeof(slcan_can_msg_t));
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

size_t slcan_can_fifo_put(slcan_can_fifo_t* fifo, const slcan_can_msg_t* msg)
{
    if(fifo->count < SLCAN_CAN_FIFO_SIZE){
        memcpy(&fifo->buf[fifo->wptr], msg, sizeof(slcan_can_msg_t));
        fifo->count ++;
        fifo->wptr ++;
        if(fifo->wptr >= SLCAN_CAN_FIFO_SIZE){
            fifo->wptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_can_fifo_get(slcan_can_fifo_t* fifo, slcan_can_msg_t* msg)
{
    if(msg && fifo->count > 0){
        memcpy(msg, &fifo->buf[fifo->rptr], sizeof(slcan_can_msg_t));
        fifo->count --;
        fifo->rptr ++;
        if(fifo->rptr >= SLCAN_CAN_FIFO_SIZE){
            fifo->rptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_can_fifo_peek(slcan_can_fifo_t* fifo, slcan_can_msg_t* msg)
{
    if(msg && fifo->count > 0){
        memcpy(msg, &fifo->buf[fifo->rptr], sizeof(slcan_can_msg_t));
        return 1;
    }
    return 0;
}

void slcan_can_fifo_data_readed(slcan_can_fifo_t* fifo, size_t data_size)
{
    fifo->count -= data_size;
    fifo->rptr += data_size;
    if(fifo->rptr >= SLCAN_CAN_FIFO_SIZE){
        fifo->rptr -= SLCAN_CAN_FIFO_SIZE;
    }
}

void slcan_can_fifo_data_written(slcan_can_fifo_t* fifo, size_t data_size)
{
    fifo->count += data_size;
    fifo->wptr += data_size;
    if(fifo->wptr >= SLCAN_CAN_FIFO_SIZE){
        fifo->wptr -= SLCAN_CAN_FIFO_SIZE;
    }
}
