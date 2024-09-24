#ifndef SLCAN_CAN_FIFO_H_
#define SLCAN_CAN_FIFO_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_can_msg.h"
#include "slcan_conf.h"


#ifndef SLCAN_CAN_FIFO_SIZE
#define SLCAN_CAN_FIFO_SIZE SLCAN_CAN_FIFO_DEFAULT_SIZE
#endif


typedef struct _Slcan_Can_Fifo {
    slcan_can_msg_t buf[SLCAN_CAN_FIFO_SIZE];
    size_t wptr;
    size_t rptr;
    size_t count;
} slcan_can_fifo_t;


EXTERN void slcan_can_fifo_init(slcan_can_fifo_t* fifo);

ALWAYS_INLINE static void slcan_can_fifo_reset(slcan_can_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

ALWAYS_INLINE static size_t slcan_can_fifo_avail(const slcan_can_fifo_t* fifo)
{
    return fifo->count;
}

ALWAYS_INLINE static size_t slcan_can_fifo_remain(const slcan_can_fifo_t* fifo)
{
    return SLCAN_CAN_FIFO_SIZE - fifo->count;
}

ALWAYS_INLINE static bool slcan_can_fifo_full(const slcan_can_fifo_t* fifo)
{
    return fifo->count == SLCAN_CAN_FIFO_SIZE;
}

ALWAYS_INLINE static bool slcan_can_fifo_empty(const slcan_can_fifo_t* fifo)
{
    return fifo->count == 0;
}

EXTERN size_t slcan_can_fifo_put(slcan_can_fifo_t* fifo, const slcan_can_msg_t* msg);

EXTERN size_t slcan_can_fifo_get(slcan_can_fifo_t* fifo, slcan_can_msg_t* msg);

EXTERN size_t slcan_can_fifo_peek(const slcan_can_fifo_t* fifo, slcan_can_msg_t* msg);

EXTERN void slcan_can_fifo_data_readed(slcan_can_fifo_t* fifo, size_t data_size);

EXTERN void slcan_can_fifo_data_written(slcan_can_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_CAN_FIFO_H_ */
