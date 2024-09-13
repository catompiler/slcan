#include "slcan_can_msg.h"


bool slcan_can_msg_is_valid(slcan_can_msg_t* msg)
{
    if(msg == NULL) return false;

    switch(msg->id_type){
        default:
            return false;
        case SLCAN_MSG_ID_NORMAL:
            if(msg->id > 0x1ff){
                return false;
            }
            break;
        case SLCAN_MSG_ID_EXTENDED:
            if(msg->id > 0x1fffffff){
                return false;
            }
            break;
    }

    switch(msg->frame_type){
        default:
            return false;
        case SLCAN_MSG_FRAME_TYPE_NORMAL:
            break;
        case SLCAN_MSG_FRAME_TYPE_RTR:
            break;
    }

    if(msg->data_size > 8){
        return false;
    }

    return true;
}

