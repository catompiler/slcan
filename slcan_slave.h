#ifndef SLCAN_SLAVE_H_
#define SLCAN_SLAVE_H_

#include "slcan.h"
#include "slcan_can_fifo.h"
#include "slcan_can_ext_fifo.h"
#include "slcan_serial_io.h"
#include "slcan_future.h"
#include "slcan_conf.h"



#define SLCAN_SLAVE_TIMESTAMP_DEFAULT 0

#define SLCAN_SLAVE_AUTO_POLL_DEFAULT 0


typedef slcan_err_t (*slcan_on_setup_can_std_t)(slcan_bit_rate_t bit_rate);
typedef slcan_err_t (*slcan_on_setup_can_btr_t)(uint8_t btr0, uint8_t btr1);
typedef slcan_err_t (*slcan_on_open_t)(void);
typedef slcan_err_t (*slcan_on_listen_t)(void);
typedef slcan_err_t (*slcan_on_close_t)(void);
typedef slcan_err_t (*slcan_on_setup_uart_t)(slcan_port_baud_t baud);


typedef struct _Slcan_Slave_Callbacks {
    slcan_on_setup_can_std_t on_setup_can_std;
    slcan_on_setup_can_btr_t on_setup_can_btr;
    slcan_on_open_t on_open;
    slcan_on_listen_t on_listen;
    slcan_on_close_t on_close;
    slcan_on_setup_uart_t on_setup_uart;
} slcan_slave_callbacks_t;



typedef enum _Slcan_Slave_Flag {
    SLCAN_SLAVE_FLAG_NONE = 0x0,
    SLCAN_SLAVE_FLAG_CONFIGURED = (1<<0),
    SLCAN_SLAVE_FLAG_OPENED = (1<<1),
    SLCAN_SLAVE_FLAG_LISTEN_ONLY = (1<<2),
    SLCAN_SLAVE_FLAG_AUTO_POLL = (1<<3),
    SLCAN_SLAVE_FLAG_TIMESTAMP = (1<<4),
} slcan_slave_flag_t;

typedef uint32_t slcan_slave_flags_t;

typedef enum _Slcan_Slave_Error {
    SLCAN_SLAVE_ERROR_NONE = 0x0,
    SLCAN_SLAVE_ERROR_IO = 0x1,
    SLCAN_SLAVE_ERROR_OVERRUN = 0x2,
    SLCAN_SLAVE_ERROR_ARBITRATION_LOST = 0x4,
} slcan_slave_error_t;

typedef uint32_t slcan_slave_errors_t;



typedef struct _Slcan_Slave {
    slcan_t* sc;
    slcan_slave_callbacks_t* cb;
    slcan_can_ext_fifo_t rxcanfifo;
    slcan_can_fifo_t txcanfifo;
    slcan_slave_flags_t flags;
    slcan_slave_errors_t errors;
} slcan_slave_t;


EXTERN slcan_err_t slcan_slave_init(slcan_slave_t* scs, slcan_t* sc, slcan_slave_callbacks_t* cb);

EXTERN void slcan_slave_deinit(slcan_slave_t* scs);

EXTERN slcan_err_t slcan_slave_poll(slcan_slave_t* scs);

EXTERN slcan_err_t slcan_slave_send_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg, slcan_future_t* future);

EXTERN slcan_err_t slcan_slave_recv_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg);


#endif /* SLCAN_SLAVE_H_ */
