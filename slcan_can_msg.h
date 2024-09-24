#ifndef SLCAN_CAN_MSG_H_
#define SLCAN_CAN_MSG_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_err.h"
#include "slcan_cmd_buf.h"


typedef enum _Slcan_Msg_Frame_Type {
    SLCAN_MSG_FRAME_TYPE_NORMAL = 0,
    SLCAN_MSG_FRAME_TYPE_RTR = 1,
} slcan_msg_frame_type_t;

typedef enum _Slcan_Msg_Id_Type {
    SLCAN_MSG_ID_NORMAL = 0, //!< 11 bit.
    SLCAN_MSG_ID_EXTENDED = 1, //!< 29 bit.
} slcan_msg_id_type_t;

#define SLCAN_CAN_MSG_DATA_SIZE 8
typedef struct _Slcan_Can_Msg {
    slcan_msg_frame_type_t frame_type;
    slcan_msg_id_type_t id_type;
    uint32_t id;
    size_t data_size;
    uint8_t data[SLCAN_CAN_MSG_DATA_SIZE];
} slcan_can_msg_t;

typedef struct _Slcan_Can_Msg_Extdata {
    bool has_timestamp;
    uint16_t timestamp;
    bool autopoll_flag;
} slcan_can_msg_extdata_t;

EXTERN bool slcan_can_msg_is_valid(const slcan_can_msg_t* msg);

EXTERN slcan_err_t slcan_can_msg_from_buf(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* cmd_buf);

EXTERN slcan_err_t slcan_can_msg_to_buf(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* cmd_buf);


#endif /* SLCAN_CAN_MSG_H_ */
