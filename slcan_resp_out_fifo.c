#include "slcan_resp_out_fifo.h"
#include <string.h>



void slcan_resp_out_fifo_init(slcan_resp_out_fifo_t* fifo)
{
    memset(fifo->buf, 0x0, SLCAN_RESP_OUT_FIFO_SIZE * sizeof(slcan_resp_out_t));
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

size_t slcan_resp_out_fifo_put(slcan_resp_out_fifo_t* fifo, const slcan_resp_out_t* resp_out)
{
    if(fifo->count < SLCAN_RESP_OUT_FIFO_SIZE){
        memcpy(&fifo->buf[fifo->wptr], resp_out, sizeof(slcan_resp_out_t));
        fifo->count ++;
        fifo->wptr ++;
        if(fifo->wptr >= SLCAN_RESP_OUT_FIFO_SIZE){
            fifo->wptr = 0;
        }
        return 1;
    }
    return 0;
}

void slcan_resp_out_fifo_unput(slcan_resp_out_fifo_t* fifo)
{
    if(fifo->count > 0){
        fifo->count --;
        if(fifo->wptr == 0){
            fifo->wptr = SLCAN_RESP_OUT_FIFO_SIZE - 1;
        }else{
            fifo->wptr --;
        }
    }
}

size_t slcan_resp_out_fifo_get(slcan_resp_out_fifo_t* fifo, slcan_resp_out_t* resp_out)
{
    if(resp_out && fifo->count > 0){
        memcpy(resp_out, &fifo->buf[fifo->rptr], sizeof(slcan_resp_out_t));
        fifo->count --;
        fifo->rptr ++;
        if(fifo->rptr >= SLCAN_RESP_OUT_FIFO_SIZE){
            fifo->rptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_resp_out_fifo_peek(const slcan_resp_out_fifo_t* fifo, slcan_resp_out_t* resp_out)
{
    if(resp_out && fifo->count > 0){
        memcpy(resp_out, &fifo->buf[fifo->rptr], sizeof(slcan_resp_out_t));
        return 1;
    }
    return 0;
}

void slcan_resp_out_fifo_data_readed(slcan_resp_out_fifo_t* fifo, size_t data_size)
{
    fifo->count -= data_size;
    fifo->rptr += data_size;
    if(fifo->rptr >= SLCAN_RESP_OUT_FIFO_SIZE){
        fifo->rptr -= SLCAN_RESP_OUT_FIFO_SIZE;
    }
}

void slcan_resp_out_fifo_data_written(slcan_resp_out_fifo_t* fifo, size_t data_size)
{
    fifo->count += data_size;
    fifo->wptr += data_size;
    if(fifo->wptr >= SLCAN_RESP_OUT_FIFO_SIZE){
        fifo->wptr -= SLCAN_RESP_OUT_FIFO_SIZE;
    }
}
