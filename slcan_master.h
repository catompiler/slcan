#ifndef SLCAN_MASTER_H_
#define SLCAN_MASTER_H_

#include "slcan.h"
#include "slcan_future.h"
#include "slcan_resp_out_fifo.h"
#include "slcan_can_fifo.h"
#include "slcan_can_ext_fifo.h"
#include "slcan_slave_status.h"
#include "slcan_conf.h"


#define SLCAN_MASTER_TIMEOUT_S_DEFAULT 0
#define SLCAN_MASTER_TIMEOUT_NS_DEFAULT 100000000

struct timespec;

typedef struct _Slcan_Master {
    slcan_t* sc;
    slcan_resp_out_fifo_t respoutfifo;
    slcan_can_ext_fifo_t rxcanfifo;
    slcan_can_fifo_t txcanfifo;
    struct timespec tp_timeout;
} slcan_master_t;


EXTERN slcan_err_t slcan_master_init(slcan_master_t* scm, slcan_t* sc);

EXTERN void slcan_master_deinit(slcan_master_t* scm);

EXTERN slcan_err_t slcan_master_set_timeout(slcan_master_t* scm, const struct timespec* tp_timeout);

EXTERN slcan_err_t slcan_master_poll(slcan_master_t* scm);

EXTERN slcan_err_t slcan_master_cmd_setup_can_std(slcan_master_t* scm, slcan_bit_rate_t bit_rate, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_setup_can_btr(slcan_master_t* scm, uint16_t btr0, uint16_t btr1, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_open(slcan_master_t* scm, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_listen(slcan_master_t* scm, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_close(slcan_master_t* scm, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_poll(slcan_master_t* scm, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_poll_all(slcan_master_t* scm, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_read_status(slcan_master_t* scm, slcan_slave_status_t* status, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_set_auto_poll(slcan_master_t* scm, bool enable, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_setup_uart(slcan_master_t* scm, slcan_port_baud_t baud, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_read_version(slcan_master_t* scm, uint8_t* hw_version, uint8_t* sw_version, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_read_sn(slcan_master_t* scm, uint16_t* sn, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_cmd_set_timestamp(slcan_master_t* scm, bool enable, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_send_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_future_t* future);

EXTERN slcan_err_t slcan_master_recv_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* extdata);

#endif /* SLCAN_MASTER_H_ */
