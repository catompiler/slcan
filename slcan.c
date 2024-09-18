#include "slcan.h"
#include "utils/utils.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>



slcan_err_t slcan_get_default_port_config(slcan_port_conf_t* conf)
{
    if(conf == NULL) return E_SLCAN_NULL_POINTER;

    conf->parity = SLCAN_PORT_PARITY_NONE;
    conf->stop_bits = SLCAN_PORT_STOP_BITS_1;
    conf->baud = SLCAN_PORT_BAUD_57600;

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_get_port_config(slcan_t* sc, slcan_port_conf_t* conf)
{
    assert(sc != NULL);

    if(conf == NULL) return E_SLCAN_NULL_POINTER;

    conf->parity = sc->port_conf.parity;
    conf->stop_bits = sc->port_conf.stop_bits;
    conf->baud = sc->port_conf.baud;

    return E_SLCAN_NO_ERROR;
}


ALWAYS_INLINE static int slcan_serial_open(slcan_t* sc, const char* serial_port_name)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->open != NULL);

    return sc->sio->open(serial_port_name);
}

ALWAYS_INLINE static int slcan_serial_configure(slcan_t* sc, slcan_port_conf_t* conf)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->configure != NULL);

    return sc->sio->configure(sc->serial_port, conf);
}

ALWAYS_INLINE static void slcan_serial_close(slcan_t* sc)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->close != NULL);

    sc->sio->close(sc->serial_port);
}

ALWAYS_INLINE static int slcan_serial_read(slcan_t* sc, void* data, size_t data_size)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->read != NULL);

    return sc->sio->read(sc->serial_port, data, data_size);
}

ALWAYS_INLINE static int slcan_serial_write(slcan_t* sc, const void* data, size_t data_size)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->write != NULL);

    return sc->sio->write(sc->serial_port, data, data_size);
}

ALWAYS_INLINE static int slcan_serial_flush(slcan_t* sc)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->flush != NULL);

    return sc->sio->flush(sc->serial_port);
}

ALWAYS_INLINE static int slcan_serial_poll(slcan_t* sc, int events, int* revents, int timeout)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->poll != NULL);

    return sc->sio->poll(sc->serial_port, events, revents, timeout);
}

ALWAYS_INLINE static int slcan_serial_nbytes(slcan_t* sc, size_t* size)
{
    assert(sc != NULL);
    assert(sc->sio != NULL);
    assert(sc->sio->nbytes != NULL);

    return sc->sio->nbytes(sc->serial_port, size);
}


static slcan_err_t slcan_process_incoming_data(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    size_t nbytes, size;

    for(;;){
        // avail size in fifo.
        size = slcan_io_fifo_write_line_size(&sc->rxiofifo);
        // fifo is full.
        if(size == 0){
            break;
        }
        // count of receiving bytes.
        res = slcan_serial_nbytes(sc, &nbytes);
        // error.
        if(res == -1){
            return E_SLCAN_IO_ERROR;
        }
        // end of transfer.
        if(nbytes == 0){
            break;
        }
        // size to read.
        nbytes = MIN(nbytes, size);
        // read data.
        res = slcan_serial_read(sc,
                                slcan_io_fifo_data_to_write(&sc->rxiofifo),
                                nbytes);
        // error.
        if(res == -1){
            return E_SLCAN_IO_ERROR;
        }
        // readed data size.
        nbytes = (size_t)res;
        // tell size to fifo.
        slcan_io_fifo_data_written(&sc->rxiofifo, nbytes);
    }

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_process_outcoming_data(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    size_t nbytes;

    for(;;){
        // avail size in fifo.
        nbytes = slcan_io_fifo_read_line_size(&sc->txiofifo);
        // fifo is empty.
        if(nbytes == 0){
            break;
        }
        // write data.
        res = slcan_serial_write(sc,
                                slcan_io_fifo_data_to_read(&sc->txiofifo),
                                nbytes);
        // error.
        if(res == -1){
            return E_SLCAN_IO_ERROR;
        }
        // readed data size.
        nbytes = (size_t)res;

        // zero bytes written;
        if(nbytes == 0){
            break;
        }

        // tell size to fifo.
        slcan_io_fifo_data_readed(&sc->txiofifo, nbytes);
    }

    return E_SLCAN_NO_ERROR;
}




/*
static bool slcan_make_msg(slcan_t* sc, slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(sc != NULL);
    assert(can_msg != NULL);
    assert(cmd != NULL);

    if(!slcan_can_msg_to_buf(can_msg, cmd)) return false;
    if(sc->flags & SLCAN_FLAG_TIMESTAMP){
        uint32_t timestamp = slcan_get_timestamp(sc);
        uint8_t* timestamp_data = slcan_cmd_buf_data_end(cmd);
        if(!slcan_number_to_hex(timestamp, timestamp_data, 4)){
            timestamp_data[0] = '0';
            timestamp_data[1] = '0';
            timestamp_data[2] = '0';
            timestamp_data[3] = '0';
        }
        slcan_cmd_buf_set_size(cmd, slcan_cmd_buf_size(cmd) + 4);
    }
    return true;
}

static bool slcan_send_next_msg(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_cmd_buf_t* answer_msg = &sc->txcmd;
    //slcan_msg_buf_reset(answer_msg); // in slcan_can_msg_to_buf(...);

    slcan_can_msg_t can_msg;

    bool sended = false;

    // if has messages.
    while(slcan_can_fifo_peek(&sc->rxcanfifo, &can_msg) != 0){
        // if msg is invalid
        if(!slcan_make_msg(sc, &can_msg, answer_msg)){
            // remove msg from fifo.
            slcan_can_fifo_data_readed(&sc->rxcanfifo, 1);
            continue;
        }

#if defined(SLCAN_DEBUG_OUTCOMING_MSGS) && SLCAN_DEBUG_OUTCOMING_MSGS == 1
    *slcan_cmd_buf_data_end(answer_msg) = '\0';
    printf("Transmitting msg: %s\n", (char*)slcan_cmd_buf_data(answer_msg));
#endif

        // add EOM.
        // if msg is too large.
        if(!slcan_cmd_buf_put(answer_msg, SLCAN_EOM_BYTE)){
            // remove msg from fifo.
            slcan_can_fifo_data_readed(&sc->rxcanfifo, 1);
            continue;
        }

        // send msg.
        if(!slcan_io_fifo_write_block(&sc->txiofifo,
                                    slcan_cmd_buf_data(answer_msg),
                                    slcan_cmd_buf_size(answer_msg))){
            sc->errors |= SLCAN_ERROR_OVERRUN;
            break;
        }
        // remove msg from fifo.
        slcan_can_fifo_data_readed(&sc->rxcanfifo, 1);

        sended = true;
        // send only one msg.
        break;
    }

    return sended;
}

static bool slcan_send_all_msgs(slcan_t* sc)
{
    bool sended = false;

    bool cur_sended;
    do {
        cur_sended = slcan_send_next_msg(sc);
        sended = sended || cur_sended;
    } while(cur_sended);

    return sended;
}
*/

/*
    if(slcan_can_fifo_full(&sc->rxcanfifo)) status |= 0x01;
    if(slcan_can_fifo_full(&sc->txcanfifo)) status |= 0x02;
    //if() status |= 0x04;
    if(sc->errors & SLCAN_ERROR_OVERRUN) status |= 0x08;
    //if() status |= 0x10;
    //if() status |= 0x20;
    //if() status |= 0x40; // arbitration lost
    if(sc->errors & SLCAN_ERROR_IO) status |= 0x80;

    // clear errors.
    sc->errors = SLCAN_ERROR_NONE;
*/


static slcan_err_t slcan_rx_cmd_buf_get_cmd(slcan_t* sc, slcan_cmd_t* cmd)
{
    assert(sc != NULL);

    slcan_cmd_buf_t* buf = &sc->rxcmd;
    slcan_err_t err;

    err = slcan_cmd_from_buf(cmd, buf);
    if(err != E_SLCAN_NO_ERROR) return err;

#if defined(SLCAN_DEBUG_INCOMING_CMDS) && SLCAN_DEBUG_INCOMING_CMDS == 1
    uint8_t* buf_data = slcan_cmd_buf_data(buf);
    buf_data[slcan_cmd_buf_size(buf)] = '\0';
    printf("Received msg: ");
    if(*buf_data == '\r'){
        printf("'\\r'\n");
    }else if(*buf_data == '\007'){
        printf("beep\n");
    }else{
        printf("%s\n", (char*)buf_data);
    }
#endif

    return E_SLCAN_NO_ERROR;
}

static slcan_err_t slcan_rx_io_fifo_get_cmd(slcan_t* sc, slcan_cmd_t* cmd)
{
    assert(sc != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    size_t size;
    uint8_t byte;
    slcan_err_t err;

    // process data rx fifo.
    for(;;){
        // get byte.
        size = slcan_io_fifo_get(&sc->rxiofifo, &byte);
        // fifo is empty.
        if(size == 0){
            break;
        }

        // put data to msg buf.
        size = slcan_cmd_buf_put(&sc->rxcmd, byte);
        // msg buf is full.
        if(size == 0){
            // reset received data.
            slcan_cmd_buf_reset(&sc->rxcmd);
            return E_SLCAN_OVERFLOW;
        }

        // end of msg.
        if((byte == SLCAN_EOM_BYTE) || (byte == SLCAN_ERR_BYTE)){
            // parse msg.
            err = slcan_rx_cmd_buf_get_cmd(sc, cmd);
            // reset processed msg.
            slcan_cmd_buf_reset(&sc->rxcmd);

            return err;
        }
    }

    return E_SLCAN_UNDERFLOW;
}

static slcan_err_t slcan_tx_io_fifo_put_cmd(slcan_t* sc, const slcan_cmd_t* cmd)
{
    assert(sc != NULL);

    if(cmd == NULL) return E_SLCAN_NULL_POINTER;

    slcan_io_fifo_t* iofifo = &sc->txiofifo;
    slcan_cmd_buf_t* buf = &sc->txcmd;
    slcan_err_t err;

    // if fifo ~ full.
    if(slcan_io_fifo_avail(iofifo) >= SLCAN_TXIOFIFO_WATERMARK){
        return E_SLCAN_OVERFLOW;
    }

    // serialize.
    err = slcan_cmd_to_buf(cmd, buf);
    if(err != E_SLCAN_NO_ERROR) return err;

    // fifo remain size too small.
    if(!slcan_io_fifo_write_block(iofifo, slcan_cmd_buf_data(buf),
                                     slcan_cmd_buf_size(buf))){
        return E_SLCAN_OVERFLOW;
    }

#if defined(SLCAN_DEBUG_INCOMING_CMDS) && SLCAN_DEBUG_INCOMING_CMDS == 1
    uint8_t* buf_data = slcan_cmd_buf_data(buf);
    buf_data[slcan_cmd_buf_size(buf)] = '\0';
    printf("Transmitting msg: ");
    if(*buf_data == '\r'){
        printf("'\\r'\n");
    }else if(*buf_data == '\007'){
        printf("beep\n");
    }else{
        printf("%s\n", (char*)buf_data);
    }
#endif

    return E_SLCAN_NO_ERROR;
}


slcan_err_t slcan_init(slcan_t* sc, slcan_serial_io_t* sio)
{
    assert(sc != NULL);

    if(sio == NULL) return -1;

    sc->sio = sio;

    slcan_get_default_port_config(&sc->port_conf);

    sc->serial_port = -1;

    slcan_io_fifo_init(&sc->txiofifo);
    slcan_io_fifo_init(&sc->rxiofifo);
    slcan_cmd_buf_init(&sc->txcmd);
    slcan_cmd_buf_init(&sc->rxcmd);

//    sc->errors = 0;
//
//    sc->flags = 0;
//
//#if defined(SLCAN_AUTO_POLL_DEFAULT)
//#if SLCAN_AUTO_POLL_DEFAULT == 1
//    sc->flags |= SLCAN_FLAG_AUTO_POLL;
//#endif
//#endif
//
//#if defined(SLCAN_TIMESTAMP_DEFAULT)
//#if SLCAN_TIMESTAMP_DEFAULT == 1
//    sc->flags |= SLCAN_FLAG_TIMESTAMP;
//#endif
//#endif

    return 0;
}

slcan_err_t slcan_open(slcan_t* sc, const char* serial_port_name)
{
    assert(sc != NULL);

    int res = slcan_serial_open(sc, serial_port_name);
    if(res == -1) return E_SLCAN_IO_ERROR;

    sc->serial_port = res;

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf)
{
    assert(sc != NULL);

    if(port_conf == NULL) port_conf = &sc->port_conf;

    int res = slcan_serial_configure(sc, port_conf);
    if(res == -1) return E_SLCAN_IO_ERROR;

    sc->port_conf.parity = port_conf->parity;
    sc->port_conf.stop_bits = port_conf->stop_bits;
    sc->port_conf.baud = port_conf->baud;

    return E_SLCAN_NO_ERROR;
}

void slcan_deinit(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_serial_close(sc);
}

slcan_err_t slcan_poll(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    slcan_err_t err;


    // poll.
    int revents = 0;
    res = slcan_serial_poll(sc, SLCAN_POLLIN | SLCAN_POLLOUT, &revents, 0);
    if(res == -1) return E_SLCAN_IO_ERROR;

    // incoming data.
    if(revents & SLCAN_POLLIN){
        err = slcan_process_incoming_data(sc);
        if(err != E_SLCAN_NO_ERROR) return err;
    }

    // outcoming data.
    if(revents & SLCAN_POLLOUT){
        err = slcan_process_outcoming_data(sc);
        if(err != E_SLCAN_NO_ERROR) return err;
    }


    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_get_cmd(slcan_t* sc, slcan_cmd_t* cmd)
{
    return slcan_rx_io_fifo_get_cmd(sc, cmd);
}

slcan_err_t slcan_put_cmd(slcan_t* sc, const slcan_cmd_t* cmd)
{
    return slcan_tx_io_fifo_put_cmd(sc, cmd);
}
