#ifndef SLCAN_H_
#define SLCAN_H_

#include "slcan_io_fifo.h"
#include "slcan_cmd_buf.h"
#include "slcan_can_msg.h"
#include "slcan_can_fifo.h"
#include "slcan_serial_io.h"
#include "slcan_get_timestamp.h"
#include "defs/defs.h"



//#define SLCAN_TIMESTAMP_DEFAULT 0

#define SLCAN_AUTO_POLL_DEFAULT 1

//#define SLCAN_DEBUG_OUTCOMING_MSGS 1

//#define SLCAN_DEBUG_INCOMING_MSGS 1



#define SLCAN_VERSION 0x1010

#define SLCAN_SERIAL_NUMBER 0x0001



typedef enum _Slcan_Flag {
    SLCAN_FLAG_NONE = 0x0,
    SLCAN_FLAG_CONFIGURED = (1<<0),
    SLCAN_FLAG_OPENED = (1<<1),
    SLCAN_FLAG_LISTEN_ONLY = (1<<2),
    SLCAN_FLAG_AUTO_POLL = (1<<3),
    SLCAN_FLAG_TIMESTAMP = (1<<4),
} slcan_flag_t;

typedef uint32_t slcan_flags_t;

typedef enum _Slcan_Error {
    SLCAN_ERROR_NONE = 0x0,
    SLCAN_ERROR_IO = 0x1,
    SLCAN_ERROR_OVERRUN = 0x2,
} slcan_error_t;

typedef uint32_t slcan_errors_t;

// End Of Message
#define SLCAN_EOM_BYTE '\r'
// OK
#define SLCAN_OK_BYTE '\r'
// OK if Auto Poll enabled
#define SLCAN_Z_BYTE 'Z'
// OK for poll all messages
#define SLCAN_A_BYTE 'A'
// Err
#define SLCAN_ERR_BYTE '\007'


//#define SLCAN_TMP_BUF_SIZE 32
typedef struct _Slcan {
    slcan_serial_io_t* sio;
    slcan_get_timestamp_t get_timestamp;
    slcan_port_conf_t port_conf;
    int serial_port;
    slcan_io_fifo_t txiofifo;
    slcan_io_fifo_t rxiofifo;
    slcan_cmd_buf_t txcmd;
    slcan_cmd_buf_t rxcmd;
    slcan_can_fifo_t txcanfifo;
    slcan_can_fifo_t rxcanfifo;
    //uint8_t tmpbuf[SLCAN_TMP_BUF_SIZE];
    slcan_flags_t flags;
    slcan_errors_t errors;
} slcan_t;


EXTERN void slcan_get_default_port_config(slcan_port_conf_t* conf);

EXTERN void slcan_get_port_config(slcan_t* sc, slcan_port_conf_t* conf);

EXTERN int slcan_init(slcan_t* sc, slcan_serial_io_t* sio, slcan_get_timestamp_t get_timestamp);

EXTERN int slcan_open(slcan_t* sc, const char* serial_port_name);

EXTERN int slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf);

EXTERN void slcan_deinit(slcan_t* sc);

EXTERN bool slcan_can_msg_from_buf(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd);

EXTERN bool slcan_can_msg_to_buf(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd);

EXTERN int slcan_poll(slcan_t* sc);

EXTERN size_t slcan_get_can_message(slcan_t* sc, slcan_can_msg_t* can_msg);

EXTERN size_t slcan_put_can_message(slcan_t* sc, const slcan_can_msg_t* can_msg);

#endif /* SLCAN_H_ */
