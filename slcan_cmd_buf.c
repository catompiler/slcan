#include "slcan_cmd_buf.h"
#include <string.h>


void slcan_cmd_buf_init(slcan_cmd_buf_t* buf)
{
    memset(buf->buf, 0x0, SLCAN_CMD_BUF_SIZE);
    buf->size = 0;
}

size_t slcan_cmd_buf_put(slcan_cmd_buf_t* buf, uint8_t data)
{
    if(buf->size < SLCAN_CMD_BUF_SIZE){
        buf->buf[buf->size] = data;
        buf->size ++;
        return 1;
    }
    return 0;
}
