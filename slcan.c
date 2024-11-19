#include "slcan.h"
#include "slcan_utils.h"
#include "slcan_port.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>



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

slcan_serial_handle_t slcan_serial_port(slcan_t* sc)
{
	assert(sc != NULL);

	return sc->serial_port;
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
        res = slcan_serial_nbytes(sc->serial_port, &nbytes);
        // error.
        if(res == SLCAN_IO_FAIL){
            return E_SLCAN_IO_ERROR;
        }
        // end of transfer.
        if(nbytes == 0){
            break;
        }
        // size to read.
        nbytes = MIN(nbytes, size);
        // read data.
        res = slcan_serial_read(sc->serial_port,
                                slcan_io_fifo_data_to_write(&sc->rxiofifo),
                                nbytes);
        // error.
        if(res == SLCAN_IO_FAIL){
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
        res = slcan_serial_write(sc->serial_port,
                                slcan_io_fifo_data_to_read(&sc->txiofifo),
                                nbytes);
        // error.
        if(res == SLCAN_IO_FAIL){
            if(errno == EAGAIN) return E_SLCAN_OVERFLOW;
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


slcan_err_t slcan_init(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_get_default_port_config(&sc->port_conf);

    slcan_io_fifo_init(&sc->txiofifo);
    slcan_io_fifo_init(&sc->rxiofifo);
    slcan_cmd_buf_init(&sc->txcmd);
    slcan_cmd_buf_init(&sc->rxcmd);

    sc->serial_port = SLCAN_IO_INVALID_HANDLE;

    return E_SLCAN_NO_ERROR;
}

void slcan_deinit(slcan_t* sc)
{
    assert(sc != NULL);
}

bool slcan_opened(slcan_t* sc)
{
    assert(sc != NULL);

    return sc->serial_port != SLCAN_IO_INVALID_HANDLE;
}

void slcan_reset(slcan_t* sc)
{
    assert(sc != NULL);

    // reset fifos.
    slcan_io_fifo_reset(&sc->txiofifo);
    slcan_io_fifo_reset(&sc->rxiofifo);
    // reset buffers.
    slcan_cmd_buf_reset(&sc->txcmd);
    slcan_cmd_buf_reset(&sc->rxcmd);
}

slcan_err_t slcan_open(slcan_t* sc, const char* serial_port_name)
{
    assert(sc != NULL);

    if(sc->serial_port != SLCAN_IO_INVALID_HANDLE){
        slcan_close(sc);
    }

    int res = slcan_serial_open(serial_port_name, &sc->serial_port);
    if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

    return E_SLCAN_NO_ERROR;
}

void slcan_close(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_serial_close(sc->serial_port);

    sc->serial_port = SLCAN_IO_INVALID_HANDLE;
}

slcan_err_t slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf)
{
    assert(sc != NULL);

    if(port_conf == NULL) port_conf = &sc->port_conf;

    int res = slcan_serial_configure(sc->serial_port, port_conf);
    if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

    sc->port_conf.parity = port_conf->parity;
    sc->port_conf.stop_bits = port_conf->stop_bits;
    sc->port_conf.baud = port_conf->baud;

    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_poll(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    slcan_err_t err;


    // poll.
    int revents = 0;
    res = slcan_serial_poll(sc->serial_port, SLCAN_POLLIN | SLCAN_POLLOUT, &revents, 0);
    if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

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

slcan_err_t slcan_poll_out(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    slcan_err_t err;


    // poll.
    int revents = 0;
    res = slcan_serial_poll(sc->serial_port, SLCAN_POLLOUT, &revents, 0);
    if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

    // outcoming data.
    if(revents & SLCAN_POLLOUT){
        err = slcan_process_outcoming_data(sc);
        if(err != E_SLCAN_NO_ERROR) return err;
    }


    return E_SLCAN_NO_ERROR;
}

slcan_err_t slcan_flush(slcan_t* sc, struct timespec* tp_timeout)
{
    assert(sc != NULL);

    if(!slcan_opened(sc)) return E_SLCAN_STATE;

    struct timespec tp_end, tp_cur;

    if(tp_timeout){
        slcan_clock_gettime(CLOCK_MONOTONIC, &tp_cur);
        slcan_timespec_add(&tp_cur, tp_timeout, &tp_end);
    }

    int res;
    slcan_err_t err;

    while(!slcan_io_fifo_empty(&sc->txiofifo)){

        res = slcan_serial_flush(sc->serial_port);
        if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

        err = slcan_poll_out(sc);
        if(err != E_SLCAN_NO_ERROR) return err;

        if(tp_timeout){
            slcan_clock_gettime(CLOCK_MONOTONIC, &tp_cur);

            if(slcan_timespec_cmp(&tp_cur, &tp_end, >)){
                return E_SLCAN_TIMEOUT;
            }
        }
    }

    res = slcan_serial_flush(sc->serial_port);
    if(res == SLCAN_IO_FAIL) return E_SLCAN_IO_ERROR;

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
