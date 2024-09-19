#include "slcan_slave.h"
#include "slcan_slave_status.h"
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
    if(cb == NULL) return E_SLCAN_NULL_POINTER;

    scs->sc = sc;
    scs->cb = cb;

    slcan_can_ext_fifo_init(&scs->rxcanfifo);
    slcan_can_ext_fifo_init(&scs->txcanfifo);

    scs->flags = SLCAN_SLAVE_FLAG_NONE;
    scs->errors = SLCAN_SLAVE_ERROR_NONE;

    return E_SLCAN_NO_ERROR;
}

void slcan_slave_deinit(slcan_slave_t* scs)
{
    assert(scs != NULL);
}

static slcan_err_t slcan_slave_send_answer(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(scs->sc == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

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

    cmd.type = (transmit_cmd->transmit.can_msg.id_type == SLCAN_MSG_ID_NORMAL) ? SLCAN_CMD_OK_AUTOPOLL : SLCAN_CMD_OK_AUTOPOLL_EXT;
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

    err = scs->cb->on_setup_can_std(bit_rate);

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

    err = scs->cb->on_setup_can_btr(btr0, btr1);

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

    err = scs->cb->on_open();

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

    err = scs->cb->on_listen();

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

    err = scs->cb->on_close();

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

    err = scs->cb->on_setup_uart(baud);

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
    if(slcan_can_ext_fifo_full(&scs->txcanfifo))      status |= SLCAN_SLAVE_STATUS_TX_FIFO_FULL;
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

    slcan_can_msg_t* can_msg = &cmd->transmit.can_msg;

    if(slcan_can_ext_fifo_put(&scs->txcanfifo, can_msg, NULL, NULL) == 0){
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

static slcan_err_t slcan_slave_send_transmit_resp_cmd(slcan_slave_t* scs, slcan_cmd_t* resp_cmd)
{
    assert(scs != NULL);

    if(resp_cmd == NULL) return E_SLCAN_NULL_POINTER;

    resp_cmd->type = slcan_cmd_type_for_can_msg(&resp_cmd->transmit.can_msg);
    resp_cmd->mode = SLCAN_CMD_MODE_RESPONSE;

    resp_cmd->transmit.extdata.autopoll_flag = false;

    if(scs->flags & SLCAN_SLAVE_FLAG_TIMESTAMP){
        if(!resp_cmd->transmit.extdata.has_timestamp){
            resp_cmd->transmit.extdata.has_timestamp = true;
            resp_cmd->transmit.extdata.timestamp = 0x0000;
        }
    }else{
        resp_cmd->transmit.extdata.has_timestamp = false;
    }

    slcan_err_t err;

    err = slcan_slave_send_answer(scs, resp_cmd);
    if(err != E_SLCAN_NO_ERROR) return err;

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

    slcan_err_t err;

    err = slcan_slave_send_transmit_resp_cmd(scs, &resp_cmd);

    if(err != E_SLCAN_NO_ERROR){
        if(err == E_SLCAN_OVERFLOW || err == E_SLCAN_OVERRUN){
            scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
            return err;
        }
        scs->errors |= SLCAN_SLAVE_ERROR_IO;
    }

    slcan_can_ext_fifo_data_readed(&scs->rxcanfifo, 1);
    if(future) slcan_future_finish(future, SLCAN_FUTURE_RESULT(err));

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_on_poll_all(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    (void) cmd;

    if(!(scs->flags & SLCAN_SLAVE_FLAG_OPENED)) return slcan_slave_send_answer_err(scs);
    if(scs->flags & SLCAN_SLAVE_FLAG_AUTO_POLL) return slcan_slave_send_answer_err(scs);

    slcan_cmd_t resp_cmd;
    slcan_future_t* future;
    slcan_err_t err;

    while(slcan_can_ext_fifo_peek(&scs->rxcanfifo, &resp_cmd.transmit.can_msg, &resp_cmd.transmit.extdata, &future) != 0){
        err = slcan_slave_send_transmit_resp_cmd(scs, &resp_cmd);

        if(err == E_SLCAN_OVERFLOW || err == E_SLCAN_OVERRUN){
            scs->errors |= SLCAN_SLAVE_ERROR_OVERRUN;
            break;
        }

        slcan_can_ext_fifo_data_readed(&scs->rxcanfifo, 1);
        if(future) slcan_future_finish(future, SLCAN_FUTURE_RESULT(err));

        if(err != E_SLCAN_NO_ERROR){
            scs->errors |= SLCAN_SLAVE_ERROR_IO;
            break;
        }
    }

    resp_cmd.type = SLCAN_CMD_POLL_ALL;
    resp_cmd.mode = SLCAN_CMD_MODE_RESPONSE;

    err = slcan_slave_send_answer(scs, &resp_cmd);

    if(err != E_SLCAN_NO_ERROR){
        return err;
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_slave_dispatch(slcan_slave_t* scs, slcan_cmd_t* cmd)
{
    assert(scs != NULL);

    if(cmd == NULL) return -1;

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
    }

    return E_SLCAN_NO_ERROR;
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
        if(err != E_SLCAN_NO_ERROR) return err;

        err = slcan_slave_dispatch(scs, &cmd);
        if(err == E_SLCAN_NO_ERROR){
            return err;
        }
    }

    return E_SLCAN_NO_ERROR;
}

static uint16_t slcan_slave_get_timestamp(slcan_slave_t* scs)
{
    (void) scs;

    struct timespec ts;

    if(slcan_clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;

    uint16_t timestamp = ts.tv_sec % 60 + ts.tv_nsec / 1000000;

    return timestamp;
}

slcan_err_t slcan_slave_send_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg, slcan_future_t* future)
{
    assert(scs != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    slcan_can_msg_extdata_t extdata;
    extdata.autopoll_flag = (scs->flags & SLCAN_SLAVE_FLAG_AUTO_POLL) != 0;
    extdata.has_timestamp = (scs->flags & SLCAN_SLAVE_FLAG_TIMESTAMP) != 0;
    if(extdata.has_timestamp){
        extdata.timestamp = slcan_slave_get_timestamp(scs);
    }else{
        extdata.timestamp = 0x0000;
    }

    if(slcan_can_ext_fifo_put(&scs->rxcanfifo, can_msg, &extdata, future) == 0){
        return E_SLCAN_OVERRUN;
    }

    if(future){
        //slcan_future_init(future);
        slcan_future_start(future);
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_slave_recv_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg)
{
    assert(scs != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    if(slcan_can_ext_fifo_get(&scs->txcanfifo, can_msg, NULL, NULL) == 0){
        return E_SLCAN_UNDERRUN;
    }

    return E_SLCAN_NO_ERROR;
}


