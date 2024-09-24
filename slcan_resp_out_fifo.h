#ifndef SLCAN_RESP_OUT_FIFO_H_
#define SLCAN_RESP_OUT_FIFO_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_resp_out.h"


#ifndef SLCAN_RESP_OUT_FIFO_SIZE
#define SLCAN_RESP_OUT_FIFO_SIZE 32
#endif


typedef struct _Slcan_Resp_Out_Fifo {
    slcan_resp_out_t buf[SLCAN_RESP_OUT_FIFO_SIZE];
    size_t wptr;
    size_t rptr;
    size_t count;
} slcan_resp_out_fifo_t;


EXTERN void slcan_resp_out_fifo_init(slcan_resp_out_fifo_t* fifo);

ALWAYS_INLINE static void slcan_resp_out_fifo_reset(slcan_resp_out_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

ALWAYS_INLINE static size_t slcan_resp_out_fifo_avail(const slcan_resp_out_fifo_t* fifo)
{
    return fifo->count;
}

ALWAYS_INLINE static size_t slcan_resp_out_fifo_remain(const slcan_resp_out_fifo_t* fifo)
{
    return SLCAN_RESP_OUT_FIFO_SIZE - fifo->count;
}

ALWAYS_INLINE static bool slcan_resp_out_fifo_full(const slcan_resp_out_fifo_t* fifo)
{
    return fifo->count == SLCAN_RESP_OUT_FIFO_SIZE;
}

EXTERN size_t slcan_resp_out_fifo_put(slcan_resp_out_fifo_t* fifo, const slcan_resp_out_t* resp_out);

EXTERN void slcan_resp_out_fifo_unput(slcan_resp_out_fifo_t* fifo);

EXTERN size_t slcan_resp_out_fifo_get(slcan_resp_out_fifo_t* fifo, slcan_resp_out_t* resp_out);

EXTERN size_t slcan_resp_out_fifo_peek(const slcan_resp_out_fifo_t* fifo, slcan_resp_out_t* resp_out);

EXTERN void slcan_resp_out_fifo_data_readed(slcan_resp_out_fifo_t* fifo, size_t data_size);

EXTERN void slcan_resp_out_fifo_data_written(slcan_resp_out_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_RESP_OUT_FIFO_H_ */
