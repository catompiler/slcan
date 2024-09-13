#ifndef SLCAN_CMD_BUF_H_
#define SLCAN_CMD_BUF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "utils/utils.h"


#ifndef SLCAN_CMD_BUF_SIZE
#define SLCAN_CMD_BUF_SIZE 32
#endif


typedef struct _Slcan_Cmd_Buf {
    uint8_t buf[SLCAN_CMD_BUF_SIZE];
    size_t size;
} slcan_cmd_buf_t;


EXTERN void slcan_cmd_buf_init(slcan_cmd_buf_t* msg);

ALWAYS_INLINE static void slcan_cmd_buf_reset(slcan_cmd_buf_t* msg)
{
    msg->size = 0;
}

ALWAYS_INLINE static uint8_t* slcan_cmd_buf_data(slcan_cmd_buf_t* msg)
{
    return msg->buf;
}

ALWAYS_INLINE static uint8_t* slcan_cmd_buf_data_end(slcan_cmd_buf_t* msg)
{
    return &msg->buf[msg->size];
}

ALWAYS_INLINE static size_t slcan_cmd_buf_size(slcan_cmd_buf_t* msg)
{
    return msg->size;
}

ALWAYS_INLINE static void slcan_cmd_buf_set_size(slcan_cmd_buf_t* msg, size_t size)
{
    msg->size = MIN(size, SLCAN_CMD_BUF_SIZE);
}

EXTERN size_t slcan_cmd_buf_put(slcan_cmd_buf_t* msg, uint8_t data);

#endif /* SLCAN_CMD_BUF_H_ */
