#include "slcan_io_fifo.h"
#include "slcan_utils.h"
#include <string.h>



void slcan_io_fifo_init(slcan_io_fifo_t* fifo)
{
    memset(fifo->buf, 0x0, SLCAN_IO_FIFO_SIZE);
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

size_t slcan_io_fifo_read_line_size(const slcan_io_fifo_t* fifo)
{
    size_t size = slcan_io_fifo_avail(fifo);
    size_t max_size = SLCAN_IO_FIFO_SIZE - fifo->rptr;

    size_t line_size = MIN(size, max_size);

    return line_size;
}

size_t slcan_io_fifo_write_line_size(const slcan_io_fifo_t* fifo)
{
    size_t size = slcan_io_fifo_remain(fifo);
    size_t max_size = SLCAN_IO_FIFO_SIZE - fifo->wptr;

    size_t line_size = MIN(size, max_size);

    return line_size;
}

size_t slcan_io_fifo_put(slcan_io_fifo_t* fifo, uint8_t data)
{
    if(fifo->count < SLCAN_IO_FIFO_SIZE){
        fifo->buf[fifo->wptr] = data;
        fifo->count ++;
        fifo->wptr ++;
        if(fifo->wptr >= SLCAN_IO_FIFO_SIZE){
            fifo->wptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_io_fifo_get(slcan_io_fifo_t* fifo, uint8_t* data)
{
    if(data && fifo->count > 0){
        *data = fifo->buf[fifo->rptr];
        fifo->count --;
        fifo->rptr ++;
        if(fifo->rptr >= SLCAN_IO_FIFO_SIZE){
            fifo->rptr = 0;
        }
        return 1;
    }
    return 0;
}

size_t slcan_io_fifo_peek(const slcan_io_fifo_t* fifo, uint8_t* data)
{
    if(data && fifo->count > 0){
        *data = fifo->buf[fifo->rptr];
        return 1;
    }
    return 0;
}

size_t slcan_io_fifo_write(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size)
{
    size_t count;
    for(count = 0; count < data_size; count ++){
        if(slcan_io_fifo_put(fifo, data[count]) == 0){
            break;
        }
    }
    return count;
}

bool slcan_io_fifo_write_block(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size)
{
    if(slcan_io_fifo_remain(fifo) < data_size) return false;

    size_t old_count = fifo->count;
    size_t old_wptr = fifo->wptr;

    if(slcan_io_fifo_write(fifo, data, data_size) != data_size){
        fifo->count = old_count;
        fifo->wptr = old_wptr;
        return false;
    }
    return true;
}

size_t slcan_io_fifo_read(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size)
{
    size_t count;
    for(count = 0; count < data_size; count ++){
        if(slcan_io_fifo_get(fifo, &data[count]) == 0){
            break;
        }
    }
    return count;
}

bool slcan_io_fifo_read_block(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size)
{
    if(slcan_io_fifo_avail(fifo) < data_size) return false;

    size_t old_count = fifo->count;
    size_t old_rptr = fifo->rptr;

    if(slcan_io_fifo_read(fifo, data, data_size) != data_size){
        fifo->count = old_count;
        fifo->rptr = old_rptr;
        return false;
    }
    return true;
}

void slcan_io_fifo_data_readed(slcan_io_fifo_t* fifo, size_t data_size)
{
    fifo->count -= data_size;
    fifo->rptr += data_size;
    if(fifo->rptr >= SLCAN_IO_FIFO_SIZE){
        fifo->rptr -= SLCAN_IO_FIFO_SIZE;
    }
}

void slcan_io_fifo_data_written(slcan_io_fifo_t* fifo, size_t data_size)
{
    fifo->count += data_size;
    fifo->wptr += data_size;
    if(fifo->wptr >= SLCAN_IO_FIFO_SIZE){
        fifo->wptr -= SLCAN_IO_FIFO_SIZE;
    }
}

