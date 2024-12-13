#include "slcan_slave.h"
#include "slcan_slave_status.h"
#include "slcan_future.h"
#include "slcan_conf.h"
#include "slcan_port.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>


slcan_err_t slcan_slave_init(slcan_slave_t* scs, slcan_t* sc, slcan_slave_callbacks_t* cb)
{
    assert(scs != NULL);

    if(sc == NULL) return E_SLCAN_NULL_POINTER;

    scs->sc = sc;
    scs->cb = cb;

    slcan_can_ext_fifo_init(&scs->rxcanfifo);
    slcan_can_fifo_init(&scs->txcanfifo);

    scs->flags = SLCAN_SLAVE_FLAG_NONE;
    scs->errors = SLCAN_SLAVE_ERROR_NONE;

#if defined(SLCAN_SLAVE_AUTO_POLL_DEFAULT)
#if SLCAN_SLAVE_AUTO_POLL_DEFAULT == 1
    scs->flags |= SLCAN_SLAVE_FLAG_AUTO_POLL;
#endif
#endif

#if defined(SLCAN_SLAVE_TIMESTAMP_DEFAULT)
#if SLCAN_SLAVE_TIMESTAMP_DEFAULT == 1
    scs->flags |= SLCAN_SLAVE_FLAG_TIMESTAMP;
#endif
#endif

    scs->user_data = NULL;

    return E_SLCAN_NO_ERROR;
}

void slcan_slave_deinit(slcan_slave_t* scs)
{
    assert(scs != NULL);
}

size_t slcan_slave_received_can_msgs_count(slcan_slave_t* scs)
{
    assert(scs != NULL);

    return slcan_can_ext_fifo_avail(&scs->rxcanfifo);
}

size_t slcan_slave_send_can_msgs_avail(slcan_slave_t* scs)
{
    assert(scs != NULL);

    return slcan_can_fifo_remain(&scs->txcanfifo);
}

ALWAYS_INLINE static void slcan_slave_future_start(slcan_future_t* future)
{
    if(future){
        slcan_future_start(future);
    }
}

ALWAYS_INLINE static void slcan_slave_future_end(slcan_future_t* future, slcan_err_t res_err)
{
    if(future){
        slcan_future_finish(future, SLCAN_FUTURE_RESULT(res_err));
    }
}

static slcan_err_t slcan_slave_send_answer(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(scs->sc == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    if(!slcan_opened(scs->sc)) return E_SLCAN_STATE;

    slcan_err_t err;

    err = slcan_put_cmd(scs->sc, cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_send_answer_ok(slcan_slave_t* scs)
{
    assert(scs != NULL);

    slcan_cmd_t cmd;

    cmd.type = SLCAN_CMD_OK;
    cmd.mode = SLCAN_CMD_MODE_RESPONSE;

    return slcan_slave_send_answer(scs, &cmd);
}

static slcan_err_t slcan_slave_send_answer_ok_autopoll(slcan_slave_t* scs, slcan_cmd_t* transmit_cmd)
{
    assert(scs != NULL);

    if(transmit_cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_cmd_t cmd;

    cmd.type = (transmit_cmd->transmit.can_msg.id_type == SLCAN_CAN_ID_NORMAL) ? SLCAN_CMD_OK_AUTOPOLL : SLCAN_CMD_OK_AUTOPOLL_EXT;
    cmd.mode = SLCAN_CMD_MODE_RESPONSE;

    return slcan_slave_send_answer(scs, &cmd);
}

static slcan_err_t slcan_slave_send_answer_err(slcan_slave_t* scs)
{
    assert(scs != NULL);

    slcan_cmd_t cmd;

    cmd.type = SLCAN_CMD_ERR;
    cmd.mode = SLCAN_CMD_MODE_RESPONSE;

    return slcan_slave_send_answer(scs, &cmd);
}

static slcan_err_t slcan_slave_on_unknown(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    return slcan_slave_send_answer_err(scs);
}

static slcan_err_t slcan_slave_on_setup_can_std(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!scs->cb || !scs->cb->on_setup_can_std) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_bit_rate_t bit_rate = cmd->setup_can_std.bit_rate;

    slcan_err_t err;

    err = scs->cb->on_setup_can_std(bit_rate, scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->flags |= SLCAN_SLAVE_FLAG_CONFIGURED;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_setup_can_btr(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!scs->cb || !scs->cb->on_setup_can_btr) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    uint8_t btr0 = cmd->setup_can_btr.btr0;
    uint8_t btr1 = cmd->setup_can_btr.btr1;

    slcan_err_t err;

    err = scs->cb->on_setup_can_btr(btr0, btr1, scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->flags |= SLCAN_SLAVE_FLAG_CONFIGURED;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_open(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!scs->cb || !scs->cb->on_open) return slcan_slave_send_answer_err(scs);
    if(!(scs->flags & SLCAN_SLAVE_FLAG_CONFIGURED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_err_t err;

    err = scs->cb->on_open(scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->flags |= SLCAN_SLAVE_FLAG_OPENED;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_listen(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!scs->cb || !scs->cb->on_listen) return slcan_slave_send_answer_err(scs);
    if(!(scs->flags & SLCAN_SLAVE_FLAG_CONFIGURED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_err_t err;

    err = scs->cb->on_listen(scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->flags |= (SLCAN_SLAVE_FLAG_OPENED | SLCAN_SLAVE_FLAG_LISTEN_ONLY);

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_close(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!scs->cb || !scs->cb->on_close) return slcan_slave_send_answer_err(scs);
    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);

    slcan_err_t err;

    err = scs->cb->on_close(scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->flags &= ~SLCAN_SLAVE_FLAG_OPENED;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_version(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    slcan_err_t err;

    slcan_cmd_t resp_cmd;
    resp_cmd.type = SLCAN_CMD_VERSION;
    resp_cmd.mode = SLCAN_CMD_MODE_RESPONSE;
    resp_cmd.version_resp.hw_version = SLCAN_HW_VERSION;
    resp_cmd.version_resp.sw_version = SLCAN_SW_VERSION;

    err = slcan_slave_send_answer(scs, &resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_sn(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    slcan_err_t err;

    slcan_cmd_t resp_cmd;
    resp_cmd.type = SLCAN_CMD_SN;
    resp_cmd.mode = SLCAN_CMD_MODE_RESPONSE;
    resp_cmd.sn_resp.sn = SLCAN_SERIAL_NUMBER;

    err = slcan_slave_send_answer(scs, &resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_set_timestamp(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_err_t err;

    bool enable = cmd->set_timestamp.value != 0;

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    if(enable){
        scs->flags |= SLCAN_SLAVE_FLAG_TIMESTAMP;
    }else{
        scs->flags &= ~SLCAN_SLAVE_FLAG_TIMESTAMP;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_set_auto_poll(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_err_t err;

    bool enable = cmd->set_auto_poll.value != 0;

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    if(enable){
        scs->flags |= SLCAN_SLAVE_FLAG_AUTO_POLL;
    }else{
        scs->flags &= ~SLCAN_SLAVE_FLAG_AUTO_POLL;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_setup_uart(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!scs->cb || !scs->cb->on_setup_uart) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    slcan_port_baud_t baud = cmd->setup_uart.baud;

    slcan_err_t err;

    err = scs->cb->on_setup_uart(baud, scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_status(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);

    uint8_t status = SLCAN_SLAVE_STATUS_NONE;

    if(slcan_can_ext_fifo_full(&scs->rxcanfifo))      status |= SLCAN_SLAVE_STATUS_RX_FIFO_FULL;
    if(slcan_can_fifo_full(&scs->txcanfifo))      status |= SLCAN_SLAVE_STATUS_TX_FIFO_FULL;
    if(scs->errors & SLCAN_SLAVE_ERROR_OVERRUN)          status |= SLCAN_SLAVE_STATUS_OVERRUN;
    if(scs->errors & SLCAN_SLAVE_ERROR_IO)               status |= SLCAN_SLAVE_STATUS_BUS_ERROR;
    if(scs->errors & SLCAN_SLAVE_ERROR_ARBITRATION_LOST) status |= SLCAN_SLAVE_STATUS_ARBITRATION_LOST;

    slcan_err_t err;

    slcan_cmd_t resp_cmd;
    resp_cmd.type = SLCAN_CMD_STATUS;
    resp_cmd.mode = SLCAN_CMD_MODE_RESPONSE;
    resp_cmd.status_resp.flags = status;

    err = slcan_slave_send_answer(scs, &resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

    scs->errors = SLCAN_SLAVE_ERROR_NONE;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_transmit(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_LISTEN_ONLY) return slcan_slave_send_answer_err(scs);

    if(slcan_can_fifo_put(&scs->txcanfifo, &cmd->transmit.can_msg, NULL) == 0){
        scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
        return slcan_slave_send_answer_err(scs);
    }

    slcan_err_t err;

    if(scs->flags & SLCAN_SLAVE_FLAG_AUTO_POLL){
        err = slcan_slave_send_answer_ok_autopoll(scs, cmd);
    }else{
        err = slcan_slave_send_answer_ok(scs);
    }
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_set_acceptance_mask(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!scs->cb || !scs->cb->on_set_acceptance_mask) return slcan_slave_send_answer_err(scs);
    if(!(scs->flags & SLCAN_SLAVE_FLAG_CONFIGURED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    uint32_t value = cmd->set_acceptance_mask.value;

    slcan_err_t err;

    err = scs->cb->on_set_acceptance_mask(value, scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_set_acceptance_filter(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(!scs->cb || !scs->cb->on_set_acceptance_filter) return slcan_slave_send_answer_err(scs);
    if(!(scs->flags & SLCAN_SLAVE_FLAG_CONFIGURED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_OPENED) return slcan_slave_send_answer_err(scs);

    uint32_t value = cmd->set_acceptance_filter.value;

    slcan_err_t err;

    err = scs->cb->on_set_acceptance_filter(value, scs->user_data);

    // fail
    if(err != E_SLCAN_NO_ERROR){
        return slcan_slave_send_answer_err(scs);
    }

    err = slcan_slave_send_answer_ok(scs);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_send_transmit_resp_cmd(slcan_slave_t* scs, slcan_cmd_t* resp_cmd)
{
    assert(scs != NULL);

    if(resp_cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_cmd_type_t cmd_type;

    cmd_type = slcan_cmd_type_for_can_msg(&resp_cmd->transmit.can_msg);
    if(cmd_type == SLCAN_CMD_UNKNOWN) return E_SLCAN_INVALID_DATA;

    resp_cmd->type = cmd_type;
    resp_cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    resp_cmd->transmit.extdata.autopoll_flag = false;
    if(scs->flags & SLCAN_SLAVE_FLAG_TIMESTAMP){
        resp_cmd->transmit.extdata.has_timestamp = true;
    }else{
        resp_cmd->transmit.extdata.has_timestamp = false;
    }

    slcan_err_t err = slcan_slave_send_answer(scs, resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_send_existing_can_msgs(slcan_slave_t* scs)
{
    assert(scs != NULL);

    slcan_err_t err;
    slcan_cmd_t resp_cmd;
    slcan_future_t* future;

    while(slcan_can_ext_fifo_peek(&scs->rxcanfifo, &resp_cmd.transmit.can_msg, &resp_cmd.transmit.extdata, &future) != 0){
        err = slcan_slave_send_transmit_resp_cmd(scs, &resp_cmd);
        if(err == E_SLCAN_OVERFLOW || err == E_SLCAN_OVERRUN){
            //scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
            // try again later.
            return err;
        }

        // remove msg from fifo.
        slcan_can_ext_fifo_data_readed(&scs->rxcanfifo, 1);
        // mesage sending done.
        slcan_slave_future_end(future, err);

        if(err != E_SLCAN_NO_ERROR){
            scs->errors |= SLCAN_SLAVE_ERROR_IO;
            return err;
        }
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_poll(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_AUTO_POLL) return slcan_slave_send_answer_err(scs);

    slcan_cmd_t resp_cmd;
    slcan_future_t* future;

    if(slcan_can_ext_fifo_peek(&scs->rxcanfifo, &resp_cmd.transmit.can_msg, &resp_cmd.transmit.extdata, &future) == 0){
        return slcan_slave_send_answer_ok(scs);
    }

    slcan_err_t err = slcan_slave_send_transmit_resp_cmd(scs, &resp_cmd);
    if(err == E_SLCAN_OVERFLOW || err == E_SLCAN_OVERRUN){
        scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
        return slcan_slave_send_answer_err(scs);
    }

    slcan_can_ext_fifo_data_readed(&scs->rxcanfifo, 1);
    slcan_slave_future_end(future, err);

    if(err != E_SLCAN_NO_ERROR){
        scs->errors |= SLCAN_SLAVE_ERROR_IO;
        return slcan_slave_send_answer_err(scs);
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_poll_all(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_AUTO_POLL) return slcan_slave_send_answer_err(scs);

    slcan_cmd_t resp_cmd;
    slcan_err_t err;

    slcan_err_t send_err = slcan_slave_send_existing_can_msgs(scs);
    if(send_err == E_SLCAN_OVERFLOW || send_err == E_SLCAN_OVERRUN){
    	scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
    }

    resp_cmd.type = SLCAN_CMD_POLL_ALL;
    resp_cmd.mode = SLCAN_CMD_MODE_RESPONSE;

    err = slcan_slave_send_answer(scs, &resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;
    
    if(send_err != E_SLCAN_NO_ERROR) return send_err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_dispatch(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    uint8_t cmd_type = cmd->type;

    switch(cmd_type){
    default:
        return slcan_slave_on_unknown(scs, cmd);
    case SLCAN_CMD_SETUP_CAN_STD:
        return slcan_slave_on_setup_can_std(scs, cmd);
    case SLCAN_CMD_SETUP_CAN_BTR:
        return slcan_slave_on_setup_can_btr(scs, cmd);
    case SLCAN_CMD_OPEN:
        return slcan_slave_on_open(scs, cmd);
    case SLCAN_CMD_LISTEN:
        return slcan_slave_on_listen(scs, cmd);
    case SLCAN_CMD_CLOSE:
        return slcan_slave_on_close(scs, cmd);
    case SLCAN_CMD_TRANSMIT:
    case SLCAN_CMD_TRANSMIT_EXT:
    case SLCAN_CMD_TRANSMIT_RTR:
    case SLCAN_CMD_TRANSMIT_RTR_EXT:
        return slcan_slave_on_transmit(scs, cmd);
    case SLCAN_CMD_POLL:
        return slcan_slave_on_poll(scs, cmd);
    case SLCAN_CMD_POLL_ALL:
        return slcan_slave_on_poll_all(scs, cmd);
    case SLCAN_CMD_STATUS:
        return slcan_slave_on_status(scs, cmd);
    case SLCAN_CMD_SET_AUTO_POLL:
        return slcan_slave_on_set_auto_poll(scs, cmd);
    case SLCAN_CMD_SETUP_UART:
        return slcan_slave_on_setup_uart(scs, cmd);
    case SLCAN_CMD_VERSION:
        return slcan_slave_on_version(scs, cmd);
    case SLCAN_CMD_SN:
        return slcan_slave_on_sn(scs, cmd);
    case SLCAN_CMD_SET_TIMESTAMP:
        return slcan_slave_on_set_timestamp(scs, cmd);
    case SLCAN_CMD_SET_ACCEPTANCE_MASK:
        return slcan_slave_on_set_acceptance_mask(scs, cmd);
    case SLCAN_CMD_SET_ACCEPTANCE_FILTER:
        return slcan_slave_on_set_acceptance_filter(scs, cmd);
    }

    return E_SLCAN_NO_ERROR;
}

ALWAYS_INLINE static bool slcan_slave_can_send_existing_messages(slcan_slave_t* scs)
{
    assert(scs != NULL);

    return scs->flags & (SLCAN_SLAVE_FLAG_OPENED | SLCAN_SLAVE_FLAG_AUTO_POLL);
}

slcan_err_t slcan_slave_poll(slcan_slave_t* scs)
{
    assert(scs != 0);

    slcan_err_t err;

#if defined(SLCAN_SLAVE_POLL_SLCAN) && SLCAN_SLAVE_POLL_SLCAN == 1
    err = slcan_poll(scs->sc);
    if(err != E_SLCAN_NO_ERROR) return err;
#endif

    slcan_cmd_t cmd;

    for(;;){
        err = slcan_get_cmd(scs->sc, &cmd);
        // if no incoming commands.
        if(err == E_SLCAN_UNDERFLOW || err == E_SLCAN_UNDERRUN) break;
        if(err != E_SLCAN_NO_ERROR) return err;

        err = slcan_slave_dispatch(scs, &cmd);
        if(err != E_SLCAN_NO_ERROR) return err;
    }

    if(slcan_slave_can_send_existing_messages(scs)){
        err = slcan_slave_send_existing_can_msgs(scs);
        if(err != E_SLCAN_NO_ERROR) return err;
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_slave_flush(slcan_slave_t* scs, struct timespec* tp_timeout)
{
    assert(scs != 0);

    struct timespec tp_end, tp_cur;
    struct timespec tp_flush, *p_tp_flush;

    if(tp_timeout){
        slcan_clock_gettime(&tp_cur);
        slcan_timespec_add(&tp_cur, tp_timeout, &tp_end);
        tp_flush.tv_sec = tp_timeout->tv_sec;
        tp_flush.tv_nsec = tp_timeout->tv_nsec;
        p_tp_flush = &tp_flush;
    }else{
    	tp_flush.tv_sec = 0;
    	tp_flush.tv_nsec = 0;
    	p_tp_flush = NULL;
    }

    slcan_err_t err;

    while(slcan_slave_can_send_existing_messages(scs) && !slcan_can_ext_fifo_empty(&scs->rxcanfifo)){

        err = slcan_flush(scs->sc, p_tp_flush);
        if(err != E_SLCAN_NO_ERROR) return err;

#if defined(SLCAN_SLAVE_POLL_SLCAN) && SLCAN_SLAVE_POLL_SLCAN == 1
        err = slcan_poll_out(scs->sc);
        if(err != E_SLCAN_NO_ERROR) return err;
#endif
        err = slcan_slave_send_existing_can_msgs(scs);
        if(err != E_SLCAN_OVERFLOW && err != E_SLCAN_OVERRUN){
        	if(err != E_SLCAN_NO_ERROR) return err;
        }

        if(tp_timeout){
            slcan_clock_gettime(&tp_cur);

            if(slcan_timespec_cmp(&tp_cur, &tp_end, >)){
                return E_SLCAN_TIMEOUT;
            }
        }

		if(p_tp_flush){
			slcan_timespec_sub(&tp_end, &tp_cur, p_tp_flush);
		}
    }

    err = slcan_flush(scs->sc, p_tp_flush);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

void slcan_slave_reset(slcan_slave_t* scs)
{
    assert(scs != NULL);

    // reset fifos.
    slcan_can_fifo_reset(&scs->txcanfifo);
    slcan_can_ext_fifo_reset(&scs->rxcanfifo);
    // reset errors.
    scs->errors = SLCAN_SLAVE_ERROR_NONE;

    // reset slcan.
    slcan_reset(scs->sc);
}

static uint16_t slcan_slave_get_timestamp(void)
{
    struct timespec ts;

    if(slcan_clock_gettime(&ts) != 0) return 0;

    uint16_t timestamp = ts.tv_sec % 60 + ts.tv_nsec / 1000000;

    return timestamp;
}

slcan_err_t slcan_slave_send_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg, slcan_future_t* future)
{
    assert(scs != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    bool empty = slcan_can_ext_fifo_empty(&scs->rxcanfifo);

    slcan_can_msg_extdata_t extdata;

    extdata.autopoll_flag = false;
    extdata.has_timestamp = false;
    extdata.timestamp = slcan_slave_get_timestamp();

    slcan_slave_future_start(future);

    if(!slcan_opened(scs->sc)){
        slcan_slave_future_end(future, E_SLCAN_STATE);
        return E_SLCAN_STATE;
    }

    if(slcan_can_ext_fifo_put(&scs->rxcanfifo, can_msg, &extdata, future) == 0){
        slcan_slave_future_end(future, E_SLCAN_OVERRUN);
        return E_SLCAN_OVERRUN;
    }

    if(empty && slcan_slave_can_send_existing_messages(scs)){
        slcan_err_t err = slcan_slave_send_existing_can_msgs(scs);
        if(err != E_SLCAN_NO_ERROR) return err;
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_slave_recv_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg)
{
    assert(scs != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    if(slcan_can_fifo_get(&scs->txcanfifo, can_msg, NULL) == 0){
        return E_SLCAN_UNDERRUN;
    }

    return E_SLCAN_NO_ERROR;
}


