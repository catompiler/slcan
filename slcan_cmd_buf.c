#include "slcan_cmd_buf.h"
#include <string.h>


void slcan_cmd_buf_init(slcan_cmd_buf_t* msg)
{
    memset(msg->buf, 0x0, SLCAN_CMD_BUF_SIZE);
    msg->size = 0;
}

size_t slcan_cmd_buf_put(slcan_cmd_buf_t* msg, uint8_t data)
{
    if(msg->size < SLCAN_CMD_BUF_SIZE){
        msg->buf[msg->size] = data;
        msg->size ++;
        return 1;
    }
    return 0;
}
