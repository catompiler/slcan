#ifndef SLCAN_RESP_OUT_H_
#define SLCAN_RESP_OUT_H_


#include <stdint.h>
#include <time.h>
#include "slcan_future.h"
#include "slcan_cmd.h"
#include "slcan_slave_status.h"


typedef struct _Slcan_Resp_Out_Setup_Can_Std {
} slcan_resp_out_setup_can_std_t;

typedef struct _Slcan_Resp_Out_Setup_Can_Btr {
} slcan_resp_out_setup_can_btr_t;

typedef struct _Slcan_Resp_Out_Open {
} slcan_resp_out_open_t;

typedef struct _Slcan_Resp_Out_Listen {
} slcan_resp_out_listen_t;

typedef struct _Slcan_Resp_Out_Close {
} slcan_resp_out_close_t;

typedef struct _Slcan_Resp_Out_Transmit {
} slcan_resp_out_transmit_t;

typedef struct _Slcan_Resp_Out_Poll {
} slcan_resp_out_poll_t;

typedef struct _Slcan_Resp_Out_Poll_All {
} slcan_resp_out_poll_all_t;

typedef struct _Slcan_Resp_Out_Status {
    slcan_slave_status_t* status;
} slcan_resp_out_status_t;

typedef struct _Slcan_Resp_Out_Set_Auto_Poll {
} slcan_resp_out_set_auto_poll_t;

typedef struct _Slcan_Resp_Out_Setup_Uart {
} slcan_resp_out_setup_uart_t;

typedef struct _Slcan_Resp_Out_Version {
    uint8_t* hw_version;
    uint8_t* sw_version;
} slcan_resp_out_version_t;

typedef struct _Slcan_Resp_Out_Sn {
    uint16_t* sn;
} slcan_resp_out_sn_t;

typedef struct _Slcan_Resp_Out_Set_Timestamp {
} slcan_resp_out_set_timestamp_t;


typedef struct _Slcan_Resp_Out {
    //slcan_cmd_t cmd_req;
    //slcan_cmd_t cmd_resp;
    slcan_cmd_type_t req_type;
    slcan_future_t* future;
    struct timespec tp_req;
    union {
        slcan_resp_out_setup_can_std_t setup_can_std;
        slcan_resp_out_setup_can_btr_t setup_can_btr;
        slcan_resp_out_open_t open;
        slcan_resp_out_listen_t listen;
        slcan_resp_out_close_t close;
        slcan_resp_out_transmit_t transmit;
        slcan_resp_out_poll_t poll;
        slcan_resp_out_poll_all_t poll_all;
        slcan_resp_out_status_t status;
        slcan_resp_out_set_auto_poll_t set_auto_poll;
        slcan_resp_out_setup_uart_t setup_uart;
        slcan_resp_out_version_t version;
        slcan_resp_out_sn_t sn;
        slcan_resp_out_set_timestamp_t set_timestamp;
    };
} slcan_resp_out_t;


#endif /* SLCAN_RESP_OUT_H_ */
