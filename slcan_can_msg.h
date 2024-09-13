#ifndef SLCAN_CAN_MSG_H_
#define SLCAN_CAN_MSG_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"


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


EXTERN bool slcan_can_msg_is_valid(slcan_can_msg_t* msg);


#endif /* SLCAN_CAN_MSG_H_ */
