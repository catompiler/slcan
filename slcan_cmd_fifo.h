#ifndef SLCAN_CMD_FIFO_H_
#define SLCAN_CMD_FIFO_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_cmd.h"


#ifndef SLCAN_CMD_FIFO_SIZE
#define SLCAN_CMD_FIFO_SIZE 32
#endif


typedef struct _Slcan_Cmd_Fifo {
    slcan_cmd_t buf[SLCAN_CMD_FIFO_SIZE];
    size_t wptr;
    size_t rptr;
    size_t count;
} slcan_cmd_fifo_t;


EXTERN void slcan_cmd_fifo_init(slcan_cmd_fifo_t* fifo);

ALWAYS_INLINE static void slcan_cmd_fifo_reset(slcan_cmd_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

ALWAYS_INLINE static size_t slcan_cmd_fifo_avail(slcan_cmd_fifo_t* fifo)
{
    return fifo->count;
}

ALWAYS_INLINE static size_t slcan_cmd_fifo_remain(slcan_cmd_fifo_t* fifo)
{
    return SLCAN_CMD_FIFO_SIZE - fifo->count;
}

ALWAYS_INLINE static bool slcan_cmd_fifo_full(slcan_cmd_fifo_t* fifo)
{
    return fifo->count == SLCAN_CMD_FIFO_SIZE;
}

EXTERN size_t slcan_cmd_fifo_put(slcan_cmd_fifo_t* fifo, const slcan_cmd_t* cmd);

EXTERN size_t slcan_cmd_fifo_get(slcan_cmd_fifo_t* fifo, slcan_cmd_t* cmd);

EXTERN size_t slcan_cmd_fifo_peek(slcan_cmd_fifo_t* fifo, slcan_cmd_t* cmd);

EXTERN void slcan_cmd_fifo_data_readed(slcan_cmd_fifo_t* fifo, size_t data_size);

EXTERN void slcan_cmd_fifo_data_written(slcan_cmd_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_CMD_FIFO_H_ */
