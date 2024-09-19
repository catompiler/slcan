#include "slcan_cmd.h"
#include <assert.h>
#include <ctype.h>
#include "slcan_utils.h"



static slcan_err_t slcan_cmd_ok_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_size(buf) != 1) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_OK;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_ok_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    (void) cmd;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_ok_z_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_size(buf) != 2) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_OK_AUTOPOLL;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_ok_z_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    (void) cmd;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_ok_Z_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_size(buf) != 2) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_OK_AUTOPOLL_EXT;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_ok_Z_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    (void) cmd;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK_AUTOPOLL_EXT) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_err_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_size(buf) != 1) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_ERR;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_err_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    (void) cmd;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_ERR) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_setup_can_std_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 2 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    uint8_t can_bitrate = digit_hex_to_num(cmd_data[0]);
    if(/*can_bitrate < 0 && */ can_bitrate > 8) return E_SLCAN_INVALID_DATA;

    cmd->type = SLCAN_CMD_SETUP_CAN_STD;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->setup_can_std.bit_rate = can_bitrate;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_setup_can_std_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SETUP_CAN_STD) == 0) return E_SLCAN_OVERFLOW;

    uint8_t can_bitrate_code = digit_num_to_hex(cmd->setup_can_std.bit_rate);
    if(slcan_cmd_buf_put(buf, can_bitrate_code) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_setup_can_btr_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 5 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check cmd data.
    for(i = 0; i < 4; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint8_t btr0 = ((digit_hex_to_num(cmd_data[0]) & 0x0f) << 4) |
                    ((digit_hex_to_num(cmd_data[1]) & 0x0f) << 0);
    uint8_t btr1 = ((digit_hex_to_num(cmd_data[2]) & 0x0f) << 4) |
                    ((digit_hex_to_num(cmd_data[3]) & 0x0f) << 0);

    cmd->type = SLCAN_CMD_SETUP_CAN_BTR;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->setup_can_btr.btr0 = btr0;
    cmd->setup_can_btr.btr1 = btr1;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_setup_can_btr_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SETUP_CAN_BTR) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->setup_can_btr.btr0 >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->setup_can_btr.btr0 >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->setup_can_btr.btr1 >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->setup_can_btr.btr1 >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_open_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_OPEN;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_open_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OPEN) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_listen_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_LISTEN;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_listen_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_LISTEN) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_close_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_CLOSE;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_close_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_CLOSE) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_transmit_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    slcan_err_t err;

    slcan_can_msg_t* can_msg = &cmd->transmit.can_msg;
    slcan_can_msg_extdata_t* extdata = &cmd->transmit.extdata;

    err = slcan_can_msg_from_buf(can_msg, extdata, buf);
    if(err != E_SLCAN_NO_ERROR) return err;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    uint8_t cmd_type = buf_data[0];

    cmd->type = cmd_type;
    cmd->mode = SLCAN_CMD_MODE_NONE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_transmit_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    slcan_err_t err;

    const slcan_can_msg_t* can_msg = &cmd->transmit.can_msg;
    const slcan_can_msg_extdata_t* extdata = &cmd->transmit.extdata;

    err = slcan_can_msg_to_buf(can_msg, extdata, buf);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_poll_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_POLL;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_poll_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_POLL) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_poll_all_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_POLL_ALL;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_poll_all_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_POLL_ALL) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_status_req_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_STATUS;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_status_req_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_STATUS) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_status_resp_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 3 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check cmd data.
    for(i = 0; i < 2; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint8_t flags = ((digit_hex_to_num(cmd_data[0]) & 0x0f) << 4) |
                     ((digit_hex_to_num(cmd_data[1]) & 0x0f) << 0);

    cmd->type = SLCAN_CMD_STATUS;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;
    cmd->status_resp.flags = flags;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_status_resp_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_STATUS) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->status_resp.flags >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->status_resp.flags >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_status_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size == 1 + 1 /* EOM */){
        return slcan_cmd_status_req_from_buf(cmd, buf);
    }else{
        return slcan_cmd_status_resp_from_buf(cmd, buf);
    }
}

static slcan_err_t slcan_cmd_status_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(cmd->mode == SLCAN_CMD_MODE_REQUEST){
        return slcan_cmd_status_req_to_buf(cmd, buf);
    }else{
        return slcan_cmd_status_resp_to_buf(cmd, buf);
    }
}


static slcan_err_t slcan_cmd_set_auto_poll_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 2 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    uint8_t value = digit_hex_to_num(cmd_data[0]);
    if(/*value < 0 && */ value > 1) return E_SLCAN_INVALID_DATA;

    cmd->type = SLCAN_CMD_SET_AUTO_POLL;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->set_auto_poll.value = value;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_set_auto_poll_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SET_AUTO_POLL) == 0) return E_SLCAN_OVERFLOW;

    uint8_t value = digit_num_to_hex(cmd->set_auto_poll.value);
    if(slcan_cmd_buf_put(buf, value) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_setup_uart_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 2 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    uint8_t uart_baud = digit_hex_to_num(cmd_data[0]);
    if(/*uart_baud < 0 && */ uart_baud > 6) return E_SLCAN_INVALID_DATA;

    cmd->type = SLCAN_CMD_SETUP_UART;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->setup_uart.baud = uart_baud;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_setup_uart_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SETUP_UART) == 0) return E_SLCAN_OVERFLOW;

    uint8_t uart_baud = digit_num_to_hex(cmd->setup_uart.baud);
    if(slcan_cmd_buf_put(buf, uart_baud) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_version_req_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_VERSION;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_version_req_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_VERSION) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_version_resp_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 5 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check cmd data.
    for(i = 0; i < 4; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint8_t hw_ver = ((digit_hex_to_num(cmd_data[0]) & 0x0f) << 4) |
                      ((digit_hex_to_num(cmd_data[1]) & 0x0f) << 0);
    uint8_t sw_ver = ((digit_hex_to_num(cmd_data[2]) & 0x0f) << 4) |
                      ((digit_hex_to_num(cmd_data[3]) & 0x0f) << 0);

    cmd->type = SLCAN_CMD_VERSION;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;
    cmd->version_resp.hw_version = hw_ver;
    cmd->version_resp.sw_version = sw_ver;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_version_resp_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_VERSION) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->version_resp.hw_version >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->version_resp.hw_version >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->version_resp.sw_version >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->version_resp.sw_version >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_version_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size == 1 + 1 /* EOM */){
        return slcan_cmd_version_req_from_buf(cmd, buf);
    }else{
        return slcan_cmd_version_resp_from_buf(cmd, buf);
    }
}

static slcan_err_t slcan_cmd_version_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(cmd->mode == SLCAN_CMD_MODE_REQUEST){
        return slcan_cmd_version_req_to_buf(cmd, buf);
    }else{
        return slcan_cmd_version_resp_to_buf(cmd, buf);
    }
}


static slcan_err_t slcan_cmd_sn_req_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 1 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    cmd->type = SLCAN_CMD_SN;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_sn_req_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SN) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_sn_resp_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 5 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    int i;

    // check cmd data.
    for(i = 0; i < 4; i ++){
        if(!isxdigit(cmd_data[i])) return E_SLCAN_INVALID_DATA;
    }

    uint16_t sn = ((digit_hex_to_num(cmd_data[0]) & 0x0f) << 12) |
                  ((digit_hex_to_num(cmd_data[1]) & 0x0f) << 8) |
                  ((digit_hex_to_num(cmd_data[2]) & 0x0f) << 4) |
                  ((digit_hex_to_num(cmd_data[3]) & 0x0f) << 0);

    cmd->type = SLCAN_CMD_SN;
    cmd->mode = SLCAN_CMD_MODE_RESPONSE;
    cmd->sn_resp.sn = sn;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_sn_resp_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SN) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->sn_resp.sn >> 12) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->sn_resp.sn >> 8) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->sn_resp.sn >> 4) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;
    if(slcan_cmd_buf_put(buf, digit_num_to_hex((cmd->sn_resp.sn >> 0) & 0x0f)) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_sn_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size == 1 + 1 /* EOM */){
        return slcan_cmd_sn_req_from_buf(cmd, buf);
    }else{
        return slcan_cmd_sn_resp_from_buf(cmd, buf);
    }
}

static slcan_err_t slcan_cmd_sn_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(cmd->mode == SLCAN_CMD_MODE_REQUEST){
        return slcan_cmd_sn_req_to_buf(cmd, buf);
    }else{
        return slcan_cmd_sn_resp_to_buf(cmd, buf);
    }
}


static slcan_err_t slcan_cmd_set_timestamp_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    size_t buf_data_size = slcan_cmd_buf_size(buf);
    if(buf_data_size != 2 + 1 /* EOM */) return E_SLCAN_INVALID_SIZE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    const uint8_t* cmd_data = &buf_data[1];

    uint8_t value = digit_hex_to_num(cmd_data[0]);
    if(/*value < 0 && */ value > 1) return E_SLCAN_INVALID_DATA;

    cmd->type = SLCAN_CMD_SET_TIMESTAMP;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->set_timestamp.value = value;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_set_timestamp_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    if(slcan_cmd_buf_put(buf, SLCAN_CMD_SET_TIMESTAMP) == 0) return E_SLCAN_OVERFLOW;

    uint8_t value = digit_num_to_hex(cmd->set_timestamp.value);
    if(slcan_cmd_buf_put(buf, value) == 0) return E_SLCAN_OVERFLOW;

    if(slcan_cmd_buf_put(buf, SLCAN_CMD_OK) == 0) return E_SLCAN_OVERFLOW;

    return E_SLCAN_NO_ERROR;
}


static slcan_err_t slcan_cmd_unknown_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);

    cmd->type = SLCAN_CMD_UNKNOWN;
    cmd->mode = SLCAN_CMD_MODE_REQUEST;
    cmd->unknown.cmd_byte = buf_data[0];

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_cmd_unknown_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    (void) cmd;
    (void) buf;

    return E_SLCAN_NO_ERROR;
}

/*
static bool slcan_cmd__from_buf(slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
}

static bool slcan_cmd__to_buf(slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
}
*/


slcan_err_t slcan_cmd_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf)
{
    assert(cmd != NULL);

    if(buf == NULL) return E_SLCAN_NULL_POINTER;
    if(slcan_cmd_buf_size(buf) == 0) return E_SLCAN_INVALID_VALUE;

    const uint8_t* buf_data = slcan_cmd_buf_data_const(buf);
    uint8_t cmd_type = buf_data[0];

    switch(cmd_type){
    default:
        return slcan_cmd_unknown_from_buf(cmd, buf);
    case SLCAN_CMD_OK:
        return slcan_cmd_ok_from_buf(cmd, buf);
    case SLCAN_CMD_OK_AUTOPOLL:
        return slcan_cmd_ok_z_from_buf(cmd, buf);
    case SLCAN_CMD_ERR:
        return slcan_cmd_err_from_buf(cmd, buf);
    case SLCAN_CMD_SETUP_CAN_STD:
        return slcan_cmd_setup_can_std_from_buf(cmd, buf);
    case SLCAN_CMD_SETUP_CAN_BTR:
        return slcan_cmd_setup_can_btr_from_buf(cmd, buf);
    case SLCAN_CMD_OPEN:
        return slcan_cmd_open_from_buf(cmd, buf);
    case SLCAN_CMD_LISTEN:
        return slcan_cmd_listen_from_buf(cmd, buf);
    case SLCAN_CMD_CLOSE:
        return slcan_cmd_close_from_buf(cmd, buf);
    case SLCAN_CMD_TRANSMIT:
    case SLCAN_CMD_TRANSMIT_EXT:
    case SLCAN_CMD_TRANSMIT_RTR:
    case SLCAN_CMD_TRANSMIT_RTR_EXT:
        return slcan_cmd_transmit_from_buf(cmd, buf);
    case SLCAN_CMD_POLL:
        return slcan_cmd_poll_from_buf(cmd, buf);
    case SLCAN_CMD_POLL_ALL:
        return slcan_cmd_poll_all_from_buf(cmd, buf);
    case SLCAN_CMD_STATUS:
        return slcan_cmd_status_from_buf(cmd, buf);
    case SLCAN_CMD_SET_AUTO_POLL:
        return slcan_cmd_set_auto_poll_from_buf(cmd, buf);
    case SLCAN_CMD_SETUP_UART:
        return slcan_cmd_setup_uart_from_buf(cmd, buf);
    case SLCAN_CMD_VERSION:
        return slcan_cmd_version_from_buf(cmd, buf);
    case SLCAN_CMD_SN:
        return slcan_cmd_sn_from_buf(cmd, buf);
    case SLCAN_CMD_SET_TIMESTAMP: // SLCAN_CMD_OK_AUTOPOLL_EXT
        if(slcan_cmd_buf_size(buf) == 2){
            return slcan_cmd_ok_Z_from_buf(cmd, buf);
        }else{
            return slcan_cmd_set_timestamp_from_buf(cmd, buf);
        }
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_cmd_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf)
{
    assert(cmd != NULL);

    if(buf == NULL) return E_SLCAN_NULL_POINTER;

    slcan_cmd_buf_reset(buf);

    uint8_t cmd_type = cmd->type;

    switch(cmd_type){
    default:
        return slcan_cmd_unknown_to_buf(cmd, buf);
    case SLCAN_CMD_OK:
        return slcan_cmd_ok_to_buf(cmd, buf);
    case SLCAN_CMD_OK_AUTOPOLL:
        return slcan_cmd_ok_z_to_buf(cmd, buf);
    case SLCAN_CMD_ERR:
        return slcan_cmd_err_to_buf(cmd, buf);
    case SLCAN_CMD_SETUP_CAN_STD:
        return slcan_cmd_setup_can_std_to_buf(cmd, buf);
    case SLCAN_CMD_SETUP_CAN_BTR:
        return slcan_cmd_setup_can_btr_to_buf(cmd, buf);
    case SLCAN_CMD_OPEN:
        return slcan_cmd_open_to_buf(cmd, buf);
    case SLCAN_CMD_LISTEN:
        return slcan_cmd_listen_to_buf(cmd, buf);
    case SLCAN_CMD_CLOSE:
        return slcan_cmd_close_to_buf(cmd, buf);
    case SLCAN_CMD_TRANSMIT:
    case SLCAN_CMD_TRANSMIT_EXT:
    case SLCAN_CMD_TRANSMIT_RTR:
    case SLCAN_CMD_TRANSMIT_RTR_EXT:
        return slcan_cmd_transmit_to_buf(cmd, buf);
    case SLCAN_CMD_POLL:
        return slcan_cmd_poll_to_buf(cmd, buf);
    case SLCAN_CMD_POLL_ALL:
        return slcan_cmd_poll_all_to_buf(cmd, buf);
    case SLCAN_CMD_STATUS:
        return slcan_cmd_status_to_buf(cmd, buf);
    case SLCAN_CMD_SET_AUTO_POLL:
        return slcan_cmd_set_auto_poll_to_buf(cmd, buf);
    case SLCAN_CMD_SETUP_UART:
        return slcan_cmd_setup_uart_to_buf(cmd, buf);
    case SLCAN_CMD_VERSION:
        return slcan_cmd_version_to_buf(cmd, buf);
    case SLCAN_CMD_SN:
        return slcan_cmd_sn_to_buf(cmd, buf);
    case SLCAN_CMD_SET_TIMESTAMP: // SLCAN_CMD_OK_AUTOPOLL_EXT
        if(cmd->mode == SLCAN_CMD_MODE_REQUEST){
            return slcan_cmd_set_timestamp_to_buf(cmd, buf);
        }else{
            return slcan_cmd_ok_Z_to_buf(cmd, buf);
        }
    }

    return E_SLCAN_NO_ERROR;
}

slcan_cmd_type_t slcan_cmd_type_for_can_msg(const slcan_can_msg_t* can_msg)
{
    if(can_msg->frame_type == SLCAN_MSG_FRAME_TYPE_NORMAL){
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return SLCAN_CMD_TRANSMIT;
        }else if(can_msg->id_type == SLCAN_MSG_ID_EXTENDED){
            return SLCAN_CMD_TRANSMIT_EXT;
        }
    }else if(can_msg->frame_type == SLCAN_MSG_FRAME_TYPE_RTR){
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return SLCAN_CMD_TRANSMIT_RTR;
        }else if(can_msg->id_type == SLCAN_MSG_ID_EXTENDED){
            return SLCAN_CMD_TRANSMIT_RTR_EXT;
        }
    }
    return SLCAN_CMD_UNKNOWN;
}

