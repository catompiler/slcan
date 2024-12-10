#include "slcan_master.h"
#include "slcan_resp_out.h"
#include "slcan_port.h"
#include "slcan_utils.h"
#include "slcan_future.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>


slcan_err_t slcan_master_init(slcan_master_t* scm, slcan_t* sc)
{
    assert(scm != NULL);

    if(sc == NULL) return E_SLCAN_NULL_POINTER;

    scm->sc = sc;
    scm->no_answers = false;

    slcan_resp_out_fifo_init(&scm->respoutfifo);

    slcan_can_ext_fifo_init(&scm->rxcanfifo);
    slcan_can_fifo_init(&scm->txcanfifo);

    scm->tp_timeout.tv_sec = SLCAN_MASTER_TIMEOUT_S_DEFAULT;
    scm->tp_timeout.tv_nsec = SLCAN_MASTER_TIMEOUT_NS_DEFAULT;

    return E_SLCAN_NO_ERROR;
}

void slcan_master_deinit(slcan_master_t* scm)
{
    assert(scm != 0);
}

size_t slcan_master_received_can_msgs_count(slcan_master_t* scm)
{
    assert(scm != 0);

    return slcan_can_ext_fifo_avail(&scm->rxcanfifo);
}

size_t slcan_master_send_can_msgs_avail(slcan_master_t* scm)
{
    assert(scm != 0);

    return slcan_can_fifo_remain(&scm->txcanfifo);
}

slcan_err_t slcan_master_set_timeout(slcan_master_t* scm, const struct timespec* tp_timeout)
{
    assert(scm != NULL);

    if(tp_timeout == NULL) return E_SLCAN_NULL_POINTER;

    scm->tp_timeout.tv_sec = tp_timeout->tv_sec;
    scm->tp_timeout.tv_nsec = tp_timeout->tv_nsec;

    return E_SLCAN_NO_ERROR;
}

bool slcan_master_no_answers(slcan_master_t* scm)
{
    return scm->no_answers;
}

void slcan_master_set_no_answers(slcan_master_t* scm, bool no_answers)
{
    scm->no_answers = no_answers;
}

ALWAYS_INLINE static void slcan_master_future_start(slcan_future_t* future)
{
    if(future){
        slcan_future_start(future);
    }
}

ALWAYS_INLINE static void slcan_master_future_end(slcan_future_t* future, slcan_err_t res_err)
{
    if(future){
        slcan_future_finish(future, SLCAN_FUTURE_RESULT(res_err));
    }
}

static slcan_err_t slcan_master_process_resp_transmit(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t err = E_SLCAN_NO_ERROR;

    if(slcan_can_ext_fifo_put(&scm->rxcanfifo, &cmd->transmit.can_msg, &cmd->transmit.extdata, NULL) == 0){
        err = E_SLCAN_OVERRUN;
    }

    if(resp_out != NULL) slcan_master_future_end(resp_out->future, err);

    return err;
}

static slcan_err_t slcan_master_process_resp_poll_all(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_POLL_ALL){
        res_err = E_SLCAN_NO_ERROR;
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_resp_ok_fail(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_OK){
        res_err = E_SLCAN_NO_ERROR;
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_resp_ok_fail_autopoll(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_OK){
        res_err = E_SLCAN_NO_ERROR;
    }else if(cmd->type == SLCAN_CMD_OK_AUTOPOLL){
        res_err = E_SLCAN_NO_ERROR;
    }else if(cmd->type == SLCAN_CMD_OK_AUTOPOLL_EXT){
        res_err = E_SLCAN_NO_ERROR;
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_resp_status(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_STATUS){
        res_err = E_SLCAN_NO_ERROR;

        if(resp_out->status.status){
            *resp_out->status.status = (slcan_slave_status_t)cmd->status_resp.flags;
        }
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_resp_version(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_VERSION){
        res_err = E_SLCAN_NO_ERROR;

        if(resp_out->version.hw_version){
            *resp_out->version.hw_version = cmd->version_resp.hw_version;
        }
        if(resp_out->version.sw_version){
            *resp_out->version.sw_version = cmd->version_resp.sw_version;
        }
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_resp_sn(slcan_master_t* scm, slcan_resp_out_t* resp_out, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_err_t res_err = E_SLCAN_UNEXPECTED;

    if(cmd->type == SLCAN_CMD_SN){
        res_err = E_SLCAN_NO_ERROR;

        if(resp_out->sn.sn){
            *resp_out->sn.sn = cmd->sn_resp.sn;
        }
    }else if(cmd->type == SLCAN_CMD_ERR){
        res_err = E_SLCAN_EXEC_FAIL;
    }

    slcan_master_future_end(resp_out->future, res_err);

    return res_err;
}

static slcan_err_t slcan_master_process_result(slcan_master_t* scm, slcan_cmd_t* cmd)
{
    assert(scm != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    bool res_cmd_is_transmit = false;

    if(cmd->type == SLCAN_CMD_TRANSMIT ||
       cmd->type == SLCAN_CMD_TRANSMIT_EXT ||
       cmd->type == SLCAN_CMD_TRANSMIT_RTR ||
       cmd->type == SLCAN_CMD_TRANSMIT_RTR_EXT){
        res_cmd_is_transmit = true;
    }

    bool has_req;
    slcan_resp_out_t resp_out;

    has_req = slcan_resp_out_fifo_peek(&scm->respoutfifo, &resp_out) != 0;
    if(!has_req){
        if(res_cmd_is_transmit) return slcan_master_process_resp_transmit(scm, NULL, cmd);
        return E_SLCAN_UNEXPECTED;
    }

    slcan_cmd_type_t req_type = resp_out.req_type;

    if(!res_cmd_is_transmit || req_type == SLCAN_CMD_POLL){
        slcan_resp_out_fifo_data_readed(&scm->respoutfifo, 1);
    }

    switch(req_type){
    default:
        return E_SLCAN_UNEXPECTED;
    case SLCAN_CMD_SETUP_CAN_STD:
    case SLCAN_CMD_SETUP_CAN_BTR:
    case SLCAN_CMD_OPEN:
    case SLCAN_CMD_LISTEN:
    case SLCAN_CMD_CLOSE:
    case SLCAN_CMD_SET_AUTO_POLL:
    case SLCAN_CMD_SETUP_UART:
    case SLCAN_CMD_SET_TIMESTAMP:
    case SLCAN_CMD_SET_ACCEPTANCE_MASK:
    case SLCAN_CMD_SET_ACCEPTANCE_FILTER:
        return slcan_master_process_resp_ok_fail(scm, &resp_out, cmd);
    case SLCAN_CMD_TRANSMIT:
    case SLCAN_CMD_TRANSMIT_EXT:
    case SLCAN_CMD_TRANSMIT_RTR:
    case SLCAN_CMD_TRANSMIT_RTR_EXT:
        return slcan_master_process_resp_ok_fail_autopoll(scm, &resp_out, cmd);
    case SLCAN_CMD_POLL:
        if(res_cmd_is_transmit){
            return slcan_master_process_resp_transmit(scm, &resp_out, cmd);
        }else{
            return slcan_master_process_resp_ok_fail(scm, &resp_out, cmd);
        }
    case SLCAN_CMD_POLL_ALL:
        if(res_cmd_is_transmit){
            return slcan_master_process_resp_transmit(scm, NULL, cmd);
        }else{
            return slcan_master_process_resp_poll_all(scm, &resp_out, cmd);
        }
    case SLCAN_CMD_STATUS:
        return slcan_master_process_resp_status(scm, &resp_out, cmd);
    case SLCAN_CMD_VERSION:
        return slcan_master_process_resp_version(scm, &resp_out, cmd);
    case SLCAN_CMD_SN:
        return slcan_master_process_resp_sn(scm, &resp_out, cmd);
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_master_send_request(slcan_master_t* scm, slcan_cmd_t* cmd, slcan_resp_out_t* resp_out)
{
    assert(scm != 0);

    if(scm->sc == NULL) return E_SLCAN_NULL_POINTER;
    if(cmd == NULL) return E_SLCAN_NULL_POINTER;
    if(resp_out == NULL) return E_SLCAN_NULL_POINTER;

    if(!slcan_opened(scm->sc)) return E_SLCAN_STATE;

    slcan_err_t err;

    slcan_clock_gettime(CLOCK_MONOTONIC, &resp_out->tp_req);
    slcan_timespec_add(&resp_out->tp_req, &scm->tp_timeout, &resp_out->tp_req);

    if(!scm->no_answers){
        if(slcan_resp_out_fifo_put(&scm->respoutfifo, resp_out) == 0){
            return E_SLCAN_OVERRUN;
        }
    }

    err = slcan_put_cmd(scm->sc, cmd);
    if(err != E_SLCAN_NO_ERROR){
        // remove from resp out queue.
        slcan_resp_out_fifo_unput(&scm->respoutfifo);
        return err;
    }

    if(scm->no_answers){
        slcan_master_future_end(resp_out->future, E_SLCAN_NO_ERROR);
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_master_send_can_msg_req(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_future_t* future)
{
    assert(scm != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;
    slcan_cmd_type_t cmd_type;

    cmd_type = slcan_cmd_type_for_can_msg(can_msg);
    if(cmd_type == SLCAN_CMD_UNKNOWN) return E_SLCAN_INVALID_DATA;

    cmd.type = cmd_type;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    memcpy(&cmd.transmit.can_msg, can_msg, sizeof(slcan_can_msg_t));
    memset(&cmd.transmit.extdata, 0x0, sizeof(slcan_can_msg_extdata_t));

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_master_send_existing_can_msgs(slcan_master_t* scm)
{
    assert(scm != NULL);

    slcan_err_t err;
    slcan_can_msg_t can_msg;
    slcan_future_t* future;

    while(slcan_can_fifo_peek(&scm->txcanfifo, &can_msg, &future)){
        err = slcan_master_send_can_msg_req(scm, &can_msg, future);
        if(err == E_SLCAN_OVERRUN || err == E_SLCAN_OVERFLOW){
            // try again later.
            return err;
        }

        // remove msg from fifo.
        slcan_can_fifo_data_readed(&scm->txcanfifo, 1);

        // if request fail.
        if(err != E_SLCAN_NO_ERROR){
            // transaction is done.
            slcan_master_future_end(future, err);
            return err;
        }
    }

    return E_SLCAN_NO_ERROR;
}

static void slcan_master_process_timeouts(slcan_master_t* scm)
{
    assert(scm != NULL);

    slcan_resp_out_t resp_out;
    struct timespec tp_cur;

    slcan_clock_gettime(CLOCK_MONOTONIC, &tp_cur);

    while(slcan_resp_out_fifo_peek(&scm->respoutfifo, &resp_out)){
        // if timeout.
        if(slcan_timespec_cmp(&resp_out.tp_req, &tp_cur, <)){
            // remove msg from fifo.
            slcan_resp_out_fifo_data_readed(&scm->respoutfifo, 1);
            // future done.
            slcan_master_future_end(resp_out.future, E_SLCAN_TIMEOUT);
            // next msg.
            continue;
        }
        // done.
        break;
    }
}

slcan_err_t slcan_master_poll(slcan_master_t* scm)
{
    assert(scm != 0);

    slcan_err_t err;

#if defined(SLCAN_MASTER_POLL_SLCAN) && SLCAN_MASTER_POLL_SLCAN == 1
    err = slcan_poll(scm->sc);
    if(err != E_SLCAN_NO_ERROR) return err;
#endif

    slcan_cmd_t cmd;

    for(;;){
        err = slcan_get_cmd(scm->sc, &cmd);
        // commands fifo empty.
        if(err == E_SLCAN_UNDERFLOW || err == E_SLCAN_UNDERRUN) break;
        if(err != E_SLCAN_NO_ERROR) return err;

        err = slcan_master_process_result(scm, &cmd);
        if(err != E_SLCAN_NO_ERROR){
            // failed response - in future result.
            if(err != E_SLCAN_EXEC_FAIL){
                return err;
            }
        }
    }

    slcan_master_process_timeouts(scm);

    err = slcan_master_send_existing_can_msgs(scm);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_master_flush(slcan_master_t* scm, struct timespec* tp_timeout)
{
    assert(scm != 0);

    struct timespec tp_end, tp_cur;
    struct timespec tp_flush, *p_tp_flush;

    if(tp_timeout){
        slcan_clock_gettime(CLOCK_MONOTONIC, &tp_cur);
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

    while(!slcan_resp_out_fifo_empty(&scm->respoutfifo)){

        err = slcan_flush(scm->sc, p_tp_flush);
        if(err != E_SLCAN_NO_ERROR) return err;

        err = slcan_master_poll(scm);
        if(err != E_SLCAN_OVERFLOW && err != E_SLCAN_OVERRUN){
        	if(err != E_SLCAN_NO_ERROR) return err;
        }

        if(tp_timeout){
            slcan_clock_gettime(CLOCK_MONOTONIC, &tp_cur);

            if(slcan_timespec_cmp(&tp_cur, &tp_end, >)){
                return E_SLCAN_TIMEOUT;
            }
        }

		if(p_tp_flush){
			slcan_timespec_sub(&tp_end, &tp_cur, p_tp_flush);
		}
    }

    err = slcan_flush(scm->sc, p_tp_flush);
    if(err != E_SLCAN_NO_ERROR) return err;

    return E_SLCAN_NO_ERROR;
}

static void slcan_master_finish_all_reqs(slcan_master_t* scm)
{
    slcan_resp_out_t resp_out;

    while(slcan_resp_out_fifo_get(&scm->respoutfifo, &resp_out) != 0){
        slcan_master_future_end(resp_out.future, E_SLCAN_CANCELED);
    }
}

void slcan_master_reset(slcan_master_t* scm)
{
    assert(scm != NULL);

    // reset fifos.
    slcan_can_fifo_reset(&scm->txcanfifo);
    slcan_can_ext_fifo_reset(&scm->rxcanfifo);

    // finish all reqs.
    slcan_master_finish_all_reqs(scm);
    // reset req fifo.
    // cleared in slcan_master_finish_all_reqs(...).
    //slcan_resp_out_fifo_reset(&scm->respoutfifo);

    // reset slcan.
    slcan_reset(scm->sc);
}

slcan_err_t slcan_master_cmd_setup_can_std(slcan_master_t* scm, slcan_bit_rate_t bit_rate, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SETUP_CAN_STD;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.setup_can_std.bit_rate = bit_rate;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_setup_can_btr(slcan_master_t* scm, uint16_t btr0, uint16_t btr1, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SETUP_CAN_BTR;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.setup_can_btr.btr0 = btr0;
    cmd.setup_can_btr.btr1 = btr1;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_open(slcan_master_t* scm, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_OPEN;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_listen(slcan_master_t* scm, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_LISTEN;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_close(slcan_master_t* scm, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_CLOSE;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

EXTERN slcan_err_t slcan_master_cmd_poll(slcan_master_t* scm, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_POLL;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

EXTERN slcan_err_t slcan_master_cmd_poll_all(slcan_master_t* scm, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_POLL_ALL;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_read_status(slcan_master_t* scm, slcan_slave_status_t* status, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_STATUS;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;
    resp_out.status.status = status;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_set_auto_poll(slcan_master_t* scm, bool enable, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SET_AUTO_POLL;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.set_auto_poll.value = enable;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_setup_uart(slcan_master_t* scm, slcan_port_baud_t baud, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SETUP_UART;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.setup_uart.baud = baud;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_read_version(slcan_master_t* scm, uint8_t* hw_version, uint8_t* sw_version, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_VERSION;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;
    resp_out.version.hw_version = hw_version;
    resp_out.version.sw_version = sw_version;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_read_sn(slcan_master_t* scm, uint16_t* sn, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SN;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;

    resp_out.req_type = cmd.type;
    resp_out.future = future;
    resp_out.sn.sn = sn;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_set_timestamp(slcan_master_t* scm, bool enable, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SET_TIMESTAMP;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.set_timestamp.value = enable;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_set_acceptance_mask(slcan_master_t* scm, uint32_t value, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SET_ACCEPTANCE_MASK;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.set_acceptance_mask.value = value;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_cmd_set_acceptance_filter(slcan_master_t* scm, uint32_t value, slcan_future_t* future)
{
    assert(scm != 0);

    slcan_cmd_t cmd;
    slcan_resp_out_t resp_out;

    cmd.type = SLCAN_CMD_SET_ACCEPTANCE_FILTER;
    cmd.mode = SLCAN_CMD_MODE_REQUEST;
    cmd.set_acceptance_filter.value = value;

    resp_out.req_type = cmd.type;
    resp_out.future = future;

    slcan_master_future_start(future);

    slcan_err_t err = slcan_master_send_request(scm, &cmd, &resp_out);
    if(err != E_SLCAN_NO_ERROR){
        slcan_master_future_end(future, err);
    }

    return err;
}

slcan_err_t slcan_master_send_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_future_t* future)
{
    assert(scm != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    bool empty = slcan_can_fifo_empty(&scm->txcanfifo);

    slcan_master_future_start(future);

    if(!slcan_opened(scm->sc)){
        slcan_master_future_end(future, E_SLCAN_STATE);
        return E_SLCAN_STATE;
    }

    if(slcan_can_fifo_put(&scm->txcanfifo, can_msg, future) == 0){
        slcan_master_future_end(future, E_SLCAN_OVERRUN);
        return E_SLCAN_OVERRUN;
    }

    if(empty){
        slcan_err_t err = slcan_master_send_existing_can_msgs(scm);
        if(err != E_SLCAN_NO_ERROR) return err;
    }

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_master_recv_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* extdata)
{
    assert(scm != NULL);

    if(can_msg == NULL) return E_SLCAN_NULL_POINTER;

    if(slcan_can_ext_fifo_get(&scm->rxcanfifo, can_msg, extdata, NULL) == 0){
        return E_SLCAN_UNDERRUN;
    }

    return E_SLCAN_NO_ERROR;
}
