#ifndef SLCAN_IO_FIFO_H_
#define SLCAN_IO_FIFO_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"


#ifndef SLCAN_IO_FIFO_SIZE
#define SLCAN_IO_FIFO_SIZE 256
#endif


typedef struct _Slcan_Io_Fifo {
    uint8_t buf[SLCAN_IO_FIFO_SIZE];
    size_t wptr;
    size_t rptr;
    size_t count;
} slcan_io_fifo_t;


EXTERN void slcan_io_fifo_init(slcan_io_fifo_t* fifo);

ALWAYS_INLINE static void slcan_io_fifo_reset(slcan_io_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

ALWAYS_INLINE static uint8_t* slcan_io_fifo_data_to_read(slcan_io_fifo_t* fifo)
{
    return &fifo->buf[fifo->rptr];
}

ALWAYS_INLINE static uint8_t* slcan_io_fifo_data_to_write(slcan_io_fifo_t* fifo)
{
    return &fifo->buf[fifo->wptr];
}

ALWAYS_INLINE static size_t slcan_io_fifo_avail(slcan_io_fifo_t* fifo)
{
    return fifo->count;
}

ALWAYS_INLINE static size_t slcan_io_fifo_remain(slcan_io_fifo_t* fifo)
{
    return SLCAN_IO_FIFO_SIZE - fifo->count;
}

ALWAYS_INLINE static bool slcan_io_fifo_full(slcan_io_fifo_t* fifo)
{
    return fifo->count == SLCAN_IO_FIFO_SIZE;
}

EXTERN size_t slcan_io_fifo_read_line_size(slcan_io_fifo_t* fifo);

EXTERN size_t slcan_io_fifo_write_line_size(slcan_io_fifo_t* fifo);

EXTERN size_t slcan_io_fifo_put(slcan_io_fifo_t* fifo, uint8_t data);

EXTERN size_t slcan_io_fifo_get(slcan_io_fifo_t* fifo, uint8_t* data);

EXTERN size_t slcan_io_fifo_peek(slcan_io_fifo_t* fifo, uint8_t* data);

EXTERN size_t slcan_io_fifo_write(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size);

EXTERN bool slcan_io_fifo_write_block(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size);

EXTERN size_t slcan_io_fifo_read(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size);

EXTERN bool slcan_io_fifo_read_block(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size);

EXTERN void slcan_io_fifo_data_readed(slcan_io_fifo_t* fifo, size_t data_size);

EXTERN void slcan_io_fifo_data_written(slcan_io_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_IO_FIFO_H_ */
