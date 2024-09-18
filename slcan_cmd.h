#ifndef SLCAN_CMD_H_
#define SLCAN_CMD_H_

#include <stdint.h>
#include <stddef.h>
#include "defs/defs.h"
#include "slcan_can_msg.h"
#include "slcan_serial_io.h"
#include "slcan_cmd_buf.h"


typedef enum _Slcan_Can_Bit_Rate {
    SLCAN_BIT_RATE_10Kbit = 0,
    SLCAN_BIT_RATE_20Kbit = 1,
    SLCAN_BIT_RATE_50Kbit = 2,
    SLCAN_BIT_RATE_100Kbit = 3,
    SLCAN_BIT_RATE_125Kbit = 4,
    SLCAN_BIT_RATE_250Kbit = 5,
    SLCAN_BIT_RATE_500Kbit = 6,
    SLCAN_BIT_RATE_800Kbit = 7,
    SLCAN_BIT_RATE_1Mbit = 8,
} slcan_bit_rate_t;


typedef struct _Slcan_Cmd_Ok {
} slcan_cmd_ok_t;

typedef struct _Slcan_Cmd_Ok_Z {
} slcan_cmd_ok_z_t;

typedef struct _Slcan_Cmd_Err {
} slcan_cmd_err_t;

typedef struct _Slcan_Cmd_Setup_Can_Std {
    slcan_bit_rate_t bit_rate;
} slcan_cmd_setup_can_std_t;

typedef struct _Slcan_Cmd_Setup_Can_Btr {
    uint8_t btr0;
    uint8_t btr1;
} slcan_cmd_setup_can_btr_t;

typedef struct _Slcan_Cmd_Open {
} slcan_cmd_open_t;

typedef struct _Slcan_Cmd_Listen {
} slcan_cmd_listen_t;

typedef struct _Slcan_Cmd_Close {
} slcan_cmd_close_t;

typedef struct _Slcan_Cmd_Transmit {
    slcan_can_msg_t can_msg;
    slcan_can_msg_extdata_t extdata;
} slcan_cmd_transmit_t;

typedef struct _Slcan_Cmd_Poll {
} slcan_cmd_poll_t;

typedef struct _Slcan_Cmd_Poll_All {
} slcan_cmd_poll_all_t;

typedef struct _Slcan_Cmd_Status_Req {
} slcan_cmd_status_req_t;

typedef struct _Slcan_Cmd_Status_Resp {
    uint8_t flags;
} slcan_cmd_status_resp_t;

typedef struct _Slcan_Cmd_Set_Auto_Poll {
    uint8_t value;
} slcan_cmd_set_auto_poll_t;

typedef struct _Slcan_Cmd_Setup_Uart {
    slcan_port_baud_t baud;
} slcan_cmd_setup_uart_t;

typedef struct _Slcan_Cmd_Version_Req {
} slcan_cmd_version_req_t;

typedef struct _Slcan_Cmd_Version_Resp {
    uint8_t hw_version;
    uint8_t sw_version;
} slcan_cmd_version_resp_t;

typedef struct _Slcan_Cmd_Sn_Req {
} slcan_cmd_sn_req_t;

typedef struct _Slcan_Cmd_Sn_Resp {
    uint16_t sn;
} slcan_cmd_sn_resp_t;

typedef struct _Slcan_Cmd_Set_Timestamp {
    uint8_t value;
} slcan_cmd_set_timestamp_t;

//typedef struct _Slcan_Cmd_ {
//
//} slcan_cmd__t;

typedef struct _Slcan_Cmd_Unknown {
    uint8_t cmd_byte;
} slcan_cmd_unknown_t;



typedef enum _Slcan_Cmd_Type {
    SLCAN_CMD_UNKNOWN = 0,
    SLCAN_CMD_OK = '\r',
    SLCAN_CMD_OK_AUTOPOLL = 'z',
    SLCAN_CMD_ERR = '\007',
    SLCAN_CMD_SETUP_CAN_STD = 'S',
    SLCAN_CMD_SETUP_CAN_BTR = 's',
    SLCAN_CMD_OPEN = 'O',
    SLCAN_CMD_LISTEN = 'L',
    SLCAN_CMD_CLOSE = 'C',
    SLCAN_CMD_TRANSMIT = 't',
    SLCAN_CMD_TRANSMIT_EXT = 'T',
    SLCAN_CMD_TRANSMIT_RTR = 'r',
    SLCAN_CMD_TRANSMIT_RTR_EXT = 'R',
    SLCAN_CMD_POLL = 'P',
    SLCAN_CMD_POLL_ALL = 'A',
    SLCAN_CMD_STATUS = 'F',
    SLCAN_CMD_SET_AUTO_POLL = 'X',
    SLCAN_CMD_SETUP_UART = 'U',
    SLCAN_CMD_VERSION = 'V',
    SLCAN_CMD_SN = 'N',
    SLCAN_CMD_SET_TIMESTAMP = 'Z',
} slcan_cmd_type_t;

typedef enum _Slcan_Cmd_Mode {
    SLCAN_CMD_MODE_NONE = 0,
    SLCAN_CMD_MODE_REQUEST = 1,
    SLCAN_CMD_MODE_RESPONSE = 2
} slcan_cmd_mode_t;

typedef struct _Slcan_Cmd {
    slcan_cmd_type_t type;
    slcan_cmd_mode_t mode;
    union {
        slcan_cmd_ok_t ok;
        slcan_cmd_ok_z_t ok_z;
        slcan_cmd_err_t err;
        slcan_cmd_unknown_t unknown;
        slcan_cmd_setup_can_std_t setup_can_std;
        slcan_cmd_setup_can_btr_t setup_can_btr;
        slcan_cmd_open_t open;
        slcan_cmd_listen_t listen;
        slcan_cmd_close_t close;
        slcan_cmd_transmit_t transmit;
        slcan_cmd_poll_t poll;
        slcan_cmd_poll_all_t poll_all;
        slcan_cmd_status_req_t status_req;
        slcan_cmd_status_resp_t status_resp;
        slcan_cmd_set_auto_poll_t set_auto_poll;
        slcan_cmd_setup_uart_t setup_uart;
        slcan_cmd_version_req_t version_req;
        slcan_cmd_version_resp_t version_resp;
        slcan_cmd_sn_req_t sn_req;
        slcan_cmd_sn_resp_t sn_resp;
        slcan_cmd_set_timestamp_t set_timestamp;
    };
} slcan_cmd_t;


EXTERN slcan_err_t slcan_cmd_from_buf(slcan_cmd_t* cmd, const slcan_cmd_buf_t* buf);

EXTERN slcan_err_t slcan_cmd_to_buf(const slcan_cmd_t* cmd, slcan_cmd_buf_t* buf);

EXTERN slcan_cmd_type_t slcan_cmd_type_for_can_msg(const slcan_can_msg_t* can_msg);

#endif /* SLCAN_CMD_H_ */
