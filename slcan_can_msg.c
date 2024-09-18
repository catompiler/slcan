#include "slcan_can_msg.h"
#include "slcan_cmd.h"
#include "slcan_utils.h"
#include <ctype.h>
#include <assert.h>


bool slcan_can_msg_is_valid(slcan_can_msg_t* msg)
{
    if(msg == NULL) return false;

    if(msg->data_size > 8) return false;

    switch(msg->id_type){
    default:
        return false;
    case SLCAN_MSG_ID_NORMAL:
        if(msg->id > 0x7ff) return false;
        break;
    case SLCAN_MSG_ID_EXTENDED:
        if(msg->id > 0x1fffffff) return false;
        break;
    }

    switch(msg->frame_type){
    default:
        return false;
    case SLCAN_MSG_FRAME_TYPE_NORMAL:
    case SLCAN_MSG_FRAME_TYPE_RTR:
        break;
    }

    return true;
}





static slcan_err_t slcan_can_msg_from_buf_t(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size < 1 + 3 + 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check id.
    for(i = 0; i < 3; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint32_t id = 0;
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);

    if(id > 0x7ff) return E_SLCAN_INVALID_DATA;

    // check data size.
    //for(i = 0; i < 3 + 1; i ++){
        if(!isxdigit(cmd_data[0])) return E_SLCAN_INVALID_DATA;
    //}

    size_t data_size;
    data_size = digit_hex_to_num(*cmd_data ++);

    if(data_size > 8) return E_SLCAN_INVALID_DATA;

    size_t msg_size = (1 + 3 + 1 + data_size + data_size + 1 /* EOM */);
    if(buf_data_size < msg_size) return E_SLCAN_INVALID_SIZE;

    // check data.
    for(i = 0; i < data_size + data_size; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_NORMAL;
    can_msg->id_type = SLCAN_MSG_ID_NORMAL;
    can_msg->id = id;
    can_msg->data_size = data_size;

    uint8_t byte;
    for(i = 0; i < data_size; i ++){
        byte  = (digit_hex_to_num(*cmd_data ++) & 0x0f) << 4;
        byte |= (digit_hex_to_num(*cmd_data ++) & 0x0f) << 0;
        can_msg->data[i] = byte;
    }

    // timestamp.
    bool ts_valid = false;
    uint16_t ts_value = 0;

    // has timestamp.
    if(buf_data_size >= (msg_size + 4)){
        // add timestamp size.
        msg_size += 4;
        // check timestamp.
        for(i = 0; i < 4; i ++){
            if(!isxdigit(cmd_data[i])) { return E_SLCAN_INVALID_DATA; }
        }
        // ts is valid.
        ts_valid = true;
        // ts value.
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);
    }

    bool autopoll = false;
    if(cmd_data[0] == SLCAN_CMD_OK_AUTOPOLL)
        { if(buf_data_size == msg_size) return E_SLCAN_INVALID_SIZE;
          msg_size ++; cmd_data ++; autopoll = true; }

    // last byte must be '\r'.
    if(cmd_data[0] != SLCAN_CMD_OK) return E_SLCAN_INVALID_DATA;

    // message data not at end.
    if(buf_data_size != msg_size) return E_SLCAN_INVALID_SIZE;

    if(ed){
        ed->has_timestamp = ts_valid;
        ed->timestamp = ts_value;
        ed->autopoll_flag = autopoll;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_from_buf_T(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size < 1 + 8 + 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check id.
    for(i = 0; i < 3; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint32_t id = 0;
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 28);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 24);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 20);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 16);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);

    if(id > 0x1fffffff) return E_SLCAN_INVALID_DATA;

    // check data size.
    //for(i = 0; i < 3 + 1; i ++){
        if(!isxdigit(cmd_data[0])) return E_SLCAN_INVALID_DATA;
    //}

    size_t data_size;
    data_size = digit_hex_to_num(*cmd_data ++);

    if(data_size > 8) return E_SLCAN_INVALID_DATA;

    size_t msg_size = (1 + 8 + 1 + data_size + data_size + 1 /* EOM */);
    if(buf_data_size < msg_size) return E_SLCAN_INVALID_SIZE;

    // check data.
    for(i = 0; i < data_size + data_size; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_NORMAL;
    can_msg->id_type = SLCAN_MSG_ID_EXTENDED;
    can_msg->id = id;
    can_msg->data_size = data_size;

    uint8_t byte;
    for(i = 0; i < data_size; i ++){
        byte  = (digit_hex_to_num(*cmd_data ++) & 0x0f) << 4;
        byte |= (digit_hex_to_num(*cmd_data ++) & 0x0f) << 0;
        can_msg->data[i] = byte;
    }

    // timestamp.
    bool ts_valid = false;
    uint16_t ts_value = 0;

    // has timestamp.
    if(buf_data_size >= (msg_size + 4)){
        // add timestamp size.
        msg_size += 4;
        // check timestamp.
        for(i = 0; i < 4; i ++){
            if(!isxdigit(cmd_data[i])) { return E_SLCAN_INVALID_DATA; }
        }
        // ts is valid.
        ts_valid = true;
        // ts value.
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);
    }

    bool autopoll = false;
    if(cmd_data[0] == SLCAN_CMD_OK_AUTOPOLL)
        { if(buf_data_size == msg_size) return E_SLCAN_INVALID_SIZE;
          msg_size ++; cmd_data ++; autopoll = true; }

    // last byte must be '\r'.
    if(cmd_data[0] != SLCAN_CMD_OK) return E_SLCAN_INVALID_DATA;

    // message data not at end.
    if(buf_data_size != msg_size) return E_SLCAN_INVALID_SIZE;

    if(ed){
        ed->has_timestamp = ts_valid;
        ed->timestamp = ts_value;
        ed->autopoll_flag = autopoll;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_from_buf_r(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size < 1 + 3 + 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check id.
    for(i = 0; i < 3; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint32_t id = 0;
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);

    if(id > 0x7ff) return E_SLCAN_INVALID_DATA;

    // check data size.
    //for(i = 0; i < 3 + 1; i ++){
        if(!isxdigit(cmd_data[0])) return E_SLCAN_INVALID_DATA;
    //}

    size_t data_size;
    data_size = digit_hex_to_num(*cmd_data ++);

    if(data_size > 8) return E_SLCAN_INVALID_DATA;

    size_t msg_size = (1 + 3 + 1 + 0 + 0 + 1 /* EOM */);
    if(buf_data_size < msg_size) return E_SLCAN_INVALID_SIZE;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_RTR;
    can_msg->id_type = SLCAN_MSG_ID_NORMAL;
    can_msg->id = id;
    can_msg->data_size = data_size;

    // timestamp.
    bool ts_valid = false;
    uint16_t ts_value = 0;

    // has timestamp.
    if(buf_data_size >= (msg_size + 4)){
        // add timestamp size.
        msg_size += 4;
        // check timestamp.
        for(i = 0; i < 4; i ++){
            if(!isxdigit(cmd_data[i])) { return E_SLCAN_INVALID_DATA; }
        }
        // ts is valid.
        ts_valid = true;
        // ts value.
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);
    }

    bool autopoll = false;
    if(cmd_data[0] == SLCAN_CMD_OK_AUTOPOLL)
        { if(buf_data_size == msg_size) return E_SLCAN_INVALID_SIZE;
          msg_size ++; cmd_data ++; autopoll = true; }

    // last byte must be '\r'.
    if(cmd_data[0] != SLCAN_CMD_OK) return E_SLCAN_INVALID_DATA;

    // message data not at end.
    if(buf_data_size != msg_size) return E_SLCAN_INVALID_SIZE;

    if(ed){
        ed->has_timestamp = ts_valid;
        ed->timestamp = ts_value;
        ed->autopoll_flag = autopoll;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_from_buf_R(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size < 1 + 8 + 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check id.
    for(i = 0; i < 3; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint32_t id = 0;
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 28);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 24);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 20);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 16);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
    id |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);

    if(id > 0x1fffffff) return E_SLCAN_INVALID_DATA;

    // check data size.
    //for(i = 0; i < 3 + 1; i ++){
        if(!isxdigit(cmd_data[0])) return E_SLCAN_INVALID_DATA;
    //}

    size_t data_size;
    data_size = digit_hex_to_num(*cmd_data ++);

    if(data_size > 8) return E_SLCAN_INVALID_DATA;

    size_t msg_size = (1 + 8 + 1 + 0 + 0 + 1 /* EOM */);
    if(buf_data_size < msg_size) return E_SLCAN_INVALID_SIZE;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_RTR;
    can_msg->id_type = SLCAN_MSG_ID_EXTENDED;
    can_msg->id = id;
    can_msg->data_size = data_size;

    // timestamp.
    bool ts_valid = false;
    uint16_t ts_value = 0;

    // has timestamp.
    if(buf_data_size >= (msg_size + 4)){
        // add timestamp size.
        msg_size += 4;
        // check timestamp.
        for(i = 0; i < 4; i ++){
            if(!isxdigit(cmd_data[i])) { return E_SLCAN_INVALID_DATA; }
        }
        // ts is valid.
        ts_valid = true;
        // ts value.
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 12);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 8);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 4);
        ts_value |= ((digit_hex_to_num(*cmd_data ++) & 0x0f) << 0);
    }

    bool autopoll = false;
    if(cmd_data[0] == SLCAN_CMD_OK_AUTOPOLL)
        { if(buf_data_size == msg_size) return E_SLCAN_INVALID_SIZE;
          msg_size ++; cmd_data ++; autopoll = true; }

    // last byte must be '\r'.
    if(cmd_data[0] != SLCAN_CMD_OK) return E_SLCAN_INVALID_DATA;

    // message data not at end.
    if(buf_data_size != msg_size) return E_SLCAN_INVALID_SIZE;

    if(ed){
        ed->has_timestamp = ts_valid;
        ed->timestamp = ts_value;
        ed->autopoll_flag = autopoll;
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_can_msg_from_buf(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* buf)
{
    if(can_msg == NULL || buf == NULL) return E_SLCAN_NULL_POINTER;
    if(slcan_cmd_buf_size(buf) == 0) return E_SLCAN_INVALID_VALUE;

    uint8_t buf_type = buf->buf[0];

    switch(buf_type){
        default:
            break;
        case 't':
            return slcan_can_msg_from_buf_t(can_msg, ed, buf);
        case 'T':
            return slcan_can_msg_from_buf_T(can_msg, ed, buf);
        case 'r':
            return slcan_can_msg_from_buf_r(can_msg, ed, buf);
        case 'R':
            return slcan_can_msg_from_buf_R(can_msg, ed, buf);
    }

    return E_SLCAN_INVALID_VALUE;
}


static slcan_err_t slcan_can_msg_to_buf_t(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    slcan_cmd_buf_reset(buf);

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_TRANSMIT) == 0) return E_SLCAN_OVERFLOW;

    // id.
    uint32_t id = can_msg->id;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    // data_size.
    if(slcan_cmd_buf_put(buf, digit_num_to_hex(can_msg->data_size)) == 0) return E_SLCAN_OVERFLOW;

    int i;
    // data.
    for(i = 0; i < can_msg->data_size; i ++){
        if(slcan_cmd_buf_put(buf, digit_num_to_hex((can_msg->data[i] >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        if(slcan_cmd_buf_put(buf, digit_num_to_hex((can_msg->data[i] >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    }

    if(ed){
        if(ed->has_timestamp){
            uint16_t ts_value = ed->timestamp;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        }
        if(ed->autopoll_flag){
            if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL) == 0) return E_SLCAN_OVERFLOW;
        }
    }

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_to_buf_T(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    slcan_cmd_buf_reset(buf);

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_TRANSMIT_EXT) == 0) return E_SLCAN_OVERFLOW;

    // id.
    uint32_t id = can_msg->id;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 28) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 24) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 20) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 16) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    // data_size.
    if(slcan_cmd_buf_put(buf, digit_num_to_hex(can_msg->data_size)) == 0) return E_SLCAN_OVERFLOW;

    int i;
    // data.
    for(i = 0; i < can_msg->data_size; i ++){
        if(slcan_cmd_buf_put(buf, digit_num_to_hex((can_msg->data[i] >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        if(slcan_cmd_buf_put(buf, digit_num_to_hex((can_msg->data[i] >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    }

    if(ed){
        if(ed->has_timestamp){
            uint16_t ts_value = ed->timestamp;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        }
        if(ed->autopoll_flag){
            if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL) == 0) return E_SLCAN_OVERFLOW;
        }
    }

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_to_buf_r(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    slcan_cmd_buf_reset(buf);

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_TRANSMIT_RTR) == 0) return E_SLCAN_OVERFLOW;

    // id.
    uint32_t id = can_msg->id;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    // data_size.
    if(slcan_cmd_buf_put(buf, digit_num_to_hex(can_msg->data_size)) == 0) return E_SLCAN_OVERFLOW;

    if(ed){
        if(ed->has_timestamp){
            uint16_t ts_value = ed->timestamp;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        }
        if(ed->autopoll_flag){
            if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL) == 0) return E_SLCAN_OVERFLOW;
        }
    }

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_can_msg_to_buf_R(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* buf)
{
    assert(can_msg != NULL);
    assert(buf != NULL);

    slcan_cmd_buf_reset(buf);

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_TRANSMIT_RTR_EXT) == 0) return E_SLCAN_OVERFLOW;

    // id.
    uint32_t id = can_msg->id;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 28) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 24) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 20) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 16) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((id >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    // data_size.
    if(slcan_cmd_buf_put(buf, digit_num_to_hex(can_msg->data_size)) == 0) return E_SLCAN_OVERFLOW;

    if(ed){
        if(ed->has_timestamp){
            uint16_t ts_value = ed->timestamp;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
            if(slcan_cmd_buf_put(buf, digit_num_to_hex((ts_value >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
        }
        if(ed->autopoll_flag){
            if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL) == 0) return E_SLCAN_OVERFLOW;
        }
    }

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_can_msg_to_buf(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* cmd)
{
    if(can_msg == NULL || cmd == NULL) return E_SLCAN_NULL_POINTER;

    if(can_msg->frame_type == SLCAN_MSG_FRAME_TYPE_NORMAL){
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return slcan_can_msg_to_buf_t(can_msg, ed, cmd);
        }else{ //SLCAN_MSG_ID_EXTENDED
            return slcan_can_msg_to_buf_T(can_msg, ed, cmd);
        }
    }else{ // SLCAN_MSG_FRAME_TYPE_RTR
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return slcan_can_msg_to_buf_r(can_msg, ed, cmd);
        }else{ //SLCAN_MSG_ID_EXTENDED
            return slcan_can_msg_to_buf_R(can_msg, ed, cmd);
        }
    }

    return E_SLCAN_INVALID_VALUE;
}
