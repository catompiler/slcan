#ifndef SLCAN_H_
#define SLCAN_H_

#include "slcan_serial_io.h"
#include "slcan_io_fifo.h"
#include "slcan_cmd_buf.h"
#include "slcan_cmd.h"
#include "slcan_err.h"
#include "slcan_port.h"
#include "defs/defs.h"


//#define SLCAN_FAIL (-1)
//
//#define SLCAN_SUCCESS (0)


//#define SLCAN_TIMESTAMP_DEFAULT 1 //0
//
//#define SLCAN_AUTO_POLL_DEFAULT 1

#define SLCAN_DEBUG_OUTCOMING_CMDS 1

#define SLCAN_DEBUG_INCOMING_CMDS 1


#define SLCAN_TXIOFIFO_WATERMARK (SLCAN_IO_FIFO_SIZE / 4 * 3)


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



typedef struct _Slcan {
    slcan_serial_io_t* sio;
    slcan_port_conf_t port_conf;
    int serial_port;
    slcan_io_fifo_t txiofifo;
    slcan_io_fifo_t rxiofifo;
    slcan_cmd_buf_t txcmd;
    slcan_cmd_buf_t rxcmd;
    //slcan_flags_t flags;
    //slcan_errors_t errors;
} slcan_t;


EXTERN slcan_err_t slcan_get_default_port_config(slcan_port_conf_t* conf);

EXTERN slcan_err_t slcan_get_port_config(slcan_t* sc, slcan_port_conf_t* conf);

EXTERN slcan_err_t slcan_init(slcan_t* sc, slcan_serial_io_t* sio);

EXTERN slcan_err_t slcan_open(slcan_t* sc, const char* serial_port_name);

EXTERN slcan_err_t slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf);

EXTERN void slcan_deinit(slcan_t* sc);

EXTERN slcan_err_t slcan_poll(slcan_t* sc);

EXTERN slcan_err_t slcan_get_cmd(slcan_t* sc, slcan_cmd_t* cmd);

EXTERN slcan_err_t slcan_put_cmd(slcan_t* sc, const slcan_cmd_t* cmd);

//ALWAYS_INLINE static slcan_flags_t slcan_flags(slcan_t* sc)
//{
//    return sc->flags;
//}
//
//ALWAYS_INLINE static void slcan_set_flags(slcan_t* sc, slcan_flags_t flags)
//{
//    sc->flags = flags;
//}
//
//ALWAYS_INLINE static slcan_errors_t slcan_errors(slcan_t* sc)
//{
//    return sc->errors;
//}
//
//ALWAYS_INLINE static void slcan_set_errors(slcan_t* sc, slcan_errors_t errors)
//{
//    sc->errors = errors;
//}

#endif /* SLCAN_H_ */
