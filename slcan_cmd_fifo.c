#include "slcan_cmd_fifo.h"
#include <string.h>



void slcan_cmd_fifo_init(slcan_cmd_fifo_t* fifo)
{
    memset(fifo->buf, 0x0, SLCAN_CMD_FIFO_SIZE * sizeof(slcan_cmd_t));
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

size_t slcan_cmd_fifo_put(slcan_cmd_fifo_t* fifo, const slcan_cmd_t* cmd)
{
    if(fifo->count < SLCAN_CMD_FIFO_SIZE){
        memcpy(&fifo->buf[fifo->wptr], cmd, sizeof(slcan_cmd_t));
        fifo->count ++;
        fifo->wptr ++;
        if(fifo->wptr >= SLCAN_CMD_FIFO_SIZE){
            fifo->wptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_cmd_fifo_get(slcan_cmd_fifo_t* fifo, slcan_cmd_t* cmd)
{
    if(cmd && fifo->count > 0){
        memcpy(cmd, &fifo->buf[fifo->rptr], sizeof(slcan_cmd_t));
        fifo->count --;
        fifo->rptr ++;
        if(fifo->rptr >= SLCAN_CMD_FIFO_SIZE){
            fifo->rptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_cmd_fifo_peek(slcan_cmd_fifo_t* fifo, slcan_cmd_t* cmd)
{
    if(cmd && fifo->count > 0){
        memcpy(cmd, &fifo->buf[fifo->rptr], sizeof(slcan_cmd_t));
        return 1;
    }
    return 0;
}

void slcan_cmd_fifo_data_readed(slcan_cmd_fifo_t* fifo, size_t data_size)
{
    fifo->count -= data_size;
    fifo->rptr += data_size;
    if(fifo->rptr >= SLCAN_CMD_FIFO_SIZE){
        fifo->rptr -= SLCAN_CMD_FIFO_SIZE;
    }
}

void slcan_cmd_fifo_data_written(slcan_cmd_fifo_t* fifo, size_t data_size)
{
    fifo->count += data_size;
    fifo->wptr += data_size;
    if(fifo->wptr >= SLCAN_CMD_FIFO_SIZE){
        fifo->wptr -= SLCAN_CMD_FIFO_SIZE;
    }
}
