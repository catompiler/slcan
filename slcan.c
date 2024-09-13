#include "slcan.h"
#include "utils/utils.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>



void slcan_get_default_port_config(slcan_port_conf_t* conf)
{
    if(conf == NULL) return;

    conf->parity = SLCAN_PORT_PARITY_NONE;
    conf->stop_bits = SLCAN_PORT_STOP_BITS_1;
    conf->baud = SLCAN_PORT_BAUD_U1;
}

void slcan_get_port_config(slcan_t* sc, slcan_port_conf_t* conf)
{
    assert(sc != NULL);

    if(conf == NULL) return;

    conf->parity = sc->port_conf.parity;
    conf->stop_bits = sc->port_conf.stop_bits;
    conf->baud = sc->port_conf.baud;
}


ALWAYS_INLINE static uint32_t slcan_get_timestamp(slcan_t* sc)
{
    (void) sc;

    assert(sc != NULL);

    if(sc->get_timestamp == NULL) return 0;

    time_t t_ms = sc->get_timestamp();

    return (uint32_t)(t_ms & 0xffff);
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


static void slcan_process_incoming_data(slcan_t* sc)
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
            sc->errors |= SLCAN_ERROR_IO;
            break;
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
            sc->errors |= SLCAN_ERROR_IO;
            break;
        }
        // readed data size.
        nbytes = (size_t)res;
        // tell size to fifo.
        slcan_io_fifo_data_written(&sc->rxiofifo, nbytes);
    }
}

static void slcan_process_outcoming_data(slcan_t* sc)
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
            sc->errors |= SLCAN_ERROR_IO;
            break;
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
}

static void slcan_send_answer_err(slcan_t* sc)
{
    assert(sc != NULL);

    size_t size;

    size = slcan_io_fifo_put(&sc->txiofifo, SLCAN_ERR_BYTE);
    if(size == 0){
        sc->errors |= SLCAN_ERROR_OVERRUN;
    }
}

static void slcan_send_answer_ok(slcan_t* sc)
{
    assert(sc != NULL);

    size_t size;

    size = slcan_io_fifo_put(&sc->txiofifo, SLCAN_OK_BYTE);
    if(size == 0){
        sc->errors |= SLCAN_ERROR_OVERRUN;
    }
}

static void slcan_send_answer_z_ok(slcan_t* sc)
{
    assert(sc != NULL);

    uint8_t answer[2] = {SLCAN_Z_BYTE, SLCAN_OK_BYTE};

    if(!slcan_io_fifo_write_block(&sc->txiofifo, answer, 2)){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
    }
}

static void slcan_send_answer_a_ok(slcan_t* sc)
{
    assert(sc != NULL);

    uint8_t answer[2] = {SLCAN_A_BYTE, SLCAN_OK_BYTE};

    if(!slcan_io_fifo_write_block(&sc->txiofifo, answer, 2)){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
    }
}

static uint8_t slcan_hex_digit_to_number(uint8_t digit)
{
    if(digit >= 'a'){
        return digit - 'a' + 0xa;
    }
    if(digit >= 'A'){
        return digit - 'A' + 0xa;
    }
    if(digit >= '0'){
        return digit - '0';
    }

    return digit;
}

static uint8_t slcan_number_to_hex_digit(uint8_t number)
{
    if(number <= 9){
        return number + '0';
    }
    if(number <= 0xf){
        return (number - 0xa) + 'a';
    }

    return number;
}

static bool slcan_hex_pair_to_number(const uint8_t* pair, uint8_t* number)
{
    if(pair == NULL || number == NULL) return false;

    uint8_t hi_digit = pair[0];
    uint8_t lo_digit = pair[1];

    if(!isxdigit(hi_digit) || !isxdigit(lo_digit)){
        return false;
    }

    uint8_t hi_num = slcan_hex_digit_to_number(hi_digit);
    uint8_t lo_num = slcan_hex_digit_to_number(lo_digit);

    *number = ((hi_num << 4) & 0xf0) | (lo_num & 0x0f);

    return true;
}

static bool slcan_number_to_hex_pair(uint8_t number, uint8_t* pair)
{
    if(pair == NULL) return false;

    uint8_t hi_num = (number >> 4) & 0xf;
    uint8_t lo_num = number & 0xf;

    uint8_t hi_digit = slcan_number_to_hex_digit(hi_num);
    uint8_t lo_digit = slcan_number_to_hex_digit(lo_num);

    pair[0] = hi_digit;
    pair[1] = lo_digit;

    return true;
}

static bool slcan_hex_to_number(uint8_t* hex, size_t digits, uint32_t* number)
{
    if(hex == NULL || number == NULL) return false;
    if(digits == 0 || digits > 8) return false;

    uint32_t shift = 0;

    uint32_t digit_i = digits - 1;

    uint8_t digit_hex;
    uint8_t digit_num;

    uint32_t res = 0;

    uint32_t i;
    for(i = 0; i < digits; i ++){

        digit_hex = hex[digit_i];
        if(!isxdigit(digit_hex)){
            return false;
        }

        digit_num = slcan_hex_digit_to_number(digit_hex);

        res |= ((uint32_t)(digit_num & 0xf)) << shift;
        shift += 4;

        digit_i --;
    }

    *number = res;

    return true;
}

static bool slcan_number_to_hex(uint32_t number, uint8_t* hex, size_t digits)
{
    if(hex == NULL) return false;
    if(digits == 0 || digits > 8) return false;

    //uint32_t shift = 0;
    uint32_t digit_i = digits - 1;

    uint8_t digit_hex;
    uint8_t digit_num;

    uint32_t i;
    for(i = 0; i < digits; i ++){

        digit_num = number & 0x0f;
        number >>= 4;

        digit_hex = slcan_number_to_hex_digit(digit_num);

        hex[digit_i] = digit_hex;

        digit_i --;
    }

    return true;
}

static bool slcan_hex_to_array(const uint8_t* hex, size_t digits, uint8_t* array, size_t array_size)
{
    if(hex == NULL || array == NULL) return false;
    if(digits == 0 || digits == 0) return false;
    if((array_size + array_size) < digits) return false;

    uint32_t digits_i = 0;
    uint32_t i = 0;

    // Odd count of digits
    if(digits & 0x1){
        if(!isxdigit(hex[0])){
            return false;
        }
        array[0] = slcan_hex_digit_to_number(hex[0]);
        digits_i ++;
        i ++;
    }

    while(digits_i < digits){
        if(!slcan_hex_pair_to_number(&hex[digits_i], &array[i])){
            return false;
        }

        digits_i += 2;
        i ++;
    }

    return true;
}

static bool slcan_array_to_hex(const uint8_t* array, size_t array_size, uint8_t* hex, size_t digits)
{
    if(hex == NULL || array == NULL) return false;
    if(array_size == 0 || digits == 0) return false;
    // count of digits == count of nibbles OR
    // count of digits == count of nibbles - 1.
    if(digits != (array_size + array_size)){
        if(digits != (array_size + array_size - 1)){
            return false;
        }
    }

    uint32_t digits_i = 0;
    uint32_t i = 0;

    // Odd count of digits
    if(digits & 0x1){
        if(array[0] > 0x0f){
            return false;
        }
        hex[0] = slcan_number_to_hex_digit(array[0]);
        digits_i ++;
        i ++;
    }

    while(digits_i < digits){
        if(!slcan_number_to_hex_pair(array[i], &hex[digits_i])){
            return false;
        }

        digits_i += 2;
        i ++;
    }

    return true;
}

static bool slcan_can_msg_from_buf_t(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);
    size_t msg_data_size = slcan_cmd_buf_size(cmd);

    if(msg_data_size < 5 + 0 /* EOM */){
        return false;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    uint8_t* id_hex = &cmd_data[0];
    uint8_t* data_size_digit_ptr = &cmd_data[3];
    uint8_t* data_hex = &cmd_data[4];

    uint8_t data_size_digit;
    size_t digits_count;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_NORMAL;
    can_msg->id_type = SLCAN_MSG_ID_NORMAL;

    if(!slcan_hex_to_number(id_hex, 3, &can_msg->id)){
        return false;
    }
    if(can_msg->id > 0x7ff){
        return false;
    }

    data_size_digit = *data_size_digit_ptr;
    if(data_size_digit < '0' || data_size_digit > '8'){
        return false;
    }

    can_msg->data_size = data_size_digit - '0';

    if(can_msg->data_size > 8){
        return false;
    }

    digits_count = can_msg->data_size + can_msg->data_size;

    if(msg_data_size != (1 + 3 + 1 + digits_count + 0 /* EOM */)){
        return false;
    }

    if(can_msg->data_size != 0){
        if(!slcan_hex_to_array(data_hex, digits_count, can_msg->data, can_msg->data_size)){
            return false;
        }
    }

    return true;
}

static bool slcan_can_msg_from_buf_T(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);
    size_t msg_data_size = slcan_cmd_buf_size(cmd);

    if(msg_data_size < 10 + 0 /* EOM */){
        return false;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    uint8_t* id_hex = &cmd_data[0];
    uint8_t* data_size_digit_ptr = &cmd_data[8];
    uint8_t* data_hex = &cmd_data[9];

    uint8_t data_size_digit;
    size_t digits_count;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_NORMAL;
    can_msg->id_type = SLCAN_MSG_ID_EXTENDED;

    if(!slcan_hex_to_number(id_hex, 8, &can_msg->id)){
        return false;
    }
    if(can_msg->id > 0x1FFFFFFF){
        return false;
    }

    data_size_digit = *data_size_digit_ptr;
    if(data_size_digit < '0' || data_size_digit > '8'){
        return false;
    }

    can_msg->data_size = data_size_digit - '0';

    if(can_msg->data_size > 8){
        return false;
    }

    digits_count = can_msg->data_size + can_msg->data_size;

    if(msg_data_size != (1 + 8 + 1 + digits_count + 0 /* EOM */)){
        return false;
    }

    if(can_msg->data_size != 0){
        if(!slcan_hex_to_array(data_hex, digits_count, can_msg->data, can_msg->data_size)){
            return false;
        }
    }

    return true;
}

static bool slcan_can_msg_from_buf_r(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);
    size_t msg_data_size = slcan_cmd_buf_size(cmd);

    if(msg_data_size != 5 + 0 /* EOM */){
        return false;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    uint8_t* id_hex = &cmd_data[0];
    uint8_t* data_size_digit_ptr = &cmd_data[3];

    uint8_t data_size_digit;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_RTR;
    can_msg->id_type = SLCAN_MSG_ID_NORMAL;

    if(!slcan_hex_to_number(id_hex, 3, &can_msg->id)){
        return false;
    }
    if(can_msg->id > 0x7ff){
        return false;
    }

    data_size_digit = *data_size_digit_ptr;
    if(data_size_digit < '0' || data_size_digit > '8'){
        return false;
    }

    can_msg->data_size = data_size_digit - '0';

    if(can_msg->data_size > 8){
        return false;
    }

    return true;
}

static bool slcan_can_msg_from_buf_R(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);
    size_t msg_data_size = slcan_cmd_buf_size(cmd);

    if(msg_data_size != 10 + 0 /* EOM */){
        return false;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    uint8_t* id_hex = &cmd_data[0];
    uint8_t* data_size_digit_ptr = &cmd_data[8];

    uint8_t data_size_digit;

    can_msg->frame_type = SLCAN_MSG_FRAME_TYPE_RTR;
    can_msg->id_type = SLCAN_MSG_ID_EXTENDED;

    if(!slcan_hex_to_number(id_hex, 8, &can_msg->id)){
        return false;
    }
    if(can_msg->id > 0x1FFFFFFF){
        return false;
    }

    data_size_digit = *data_size_digit_ptr;
    if(data_size_digit < '0' || data_size_digit > '8'){
        return false;
    }

    can_msg->data_size = data_size_digit - '0';

    if(can_msg->data_size > 8){
        return false;
    }

    return true;
}

bool slcan_can_msg_from_buf(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    if(can_msg == NULL || cmd == NULL) return false;
    if(slcan_cmd_buf_size(cmd) == 0) return false;

    uint8_t buf_type = cmd->buf[0];

    switch(buf_type){
        default:
            break;
        case 't':
            return slcan_can_msg_from_buf_t(can_msg, cmd);
        case 'T':
            return slcan_can_msg_from_buf_T(can_msg, cmd);
        case 'r':
            return slcan_can_msg_from_buf_r(can_msg, cmd);
        case 'R':
            return slcan_can_msg_from_buf_R(can_msg, cmd);
    }

    return false;
}

static bool slcan_can_msg_to_buf_t(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    slcan_cmd_buf_reset(cmd);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);

    size_t digits_count = can_msg->data_size + can_msg->data_size;

    uint8_t* cmd_ptr = &msg_data[0];
    uint8_t* id_ptr = &msg_data[1];
    uint8_t* data_size_ptr = &msg_data[4];
    uint8_t* data_ptr = &msg_data[5];
    //uint8_t* end_ptr = &data_ptr[digits_count];

    *cmd_ptr = 't';

    if(!slcan_number_to_hex(can_msg->id, id_ptr, 3)){
        return false;
    }

    *data_size_ptr = '0' + can_msg->data_size;

    if(can_msg->data_size != 0){
        if(!slcan_array_to_hex(can_msg->data, can_msg->data_size, data_ptr, digits_count)){
            return false;
        }
    }

    //*end_ptr = SLCAN_EOM_BYTE;

    slcan_cmd_buf_set_size(cmd, 1 + 3 + 1 + digits_count + 0 /* EOM */);

    return true;
}

static bool slcan_can_msg_to_buf_T(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    slcan_cmd_buf_reset(cmd);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);

    size_t digits_count = can_msg->data_size + can_msg->data_size;

    uint8_t* cmd_ptr = &msg_data[0];
    uint8_t* id_ptr = &msg_data[1];
    uint8_t* data_size_ptr = &msg_data[9];
    uint8_t* data_ptr = &msg_data[10];
    //uint8_t* end_ptr = &data_ptr[digits_count];

    *cmd_ptr = 'T';

    if(!slcan_number_to_hex(can_msg->id, id_ptr, 8)){
        return false;
    }

    *data_size_ptr = '0' + can_msg->data_size;

    if(can_msg->data_size != 0){
        if(!slcan_array_to_hex(can_msg->data, can_msg->data_size, data_ptr, digits_count)){
            return false;
        }
    }

    //*end_ptr = SLCAN_EOM_BYTE;

    slcan_cmd_buf_set_size(cmd, 1 + 8 + 1 + digits_count + 0 /* EOM */);

    return true;
}

static bool slcan_can_msg_to_buf_r(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    slcan_cmd_buf_reset(cmd);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);

    uint8_t* cmd_ptr = &msg_data[0];
    uint8_t* id_ptr = &msg_data[1];
    uint8_t* data_size_ptr = &msg_data[4];
    //uint8_t* end_ptr = &msg_data[5];

    *cmd_ptr = 'r';

    if(!slcan_number_to_hex(can_msg->id, id_ptr, 3)){
        return false;
    }

    *data_size_ptr = '0' + can_msg->data_size;

    //*end_ptr = SLCAN_EOM_BYTE;

    slcan_cmd_buf_set_size(cmd, 1 + 3 + 1 + 0 + 0 /* EOM */);

    return true;
}

static bool slcan_can_msg_to_buf_R(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    assert(can_msg != NULL);
    assert(cmd != NULL);

    slcan_cmd_buf_reset(cmd);

    uint8_t* msg_data = slcan_cmd_buf_data(cmd);

    uint8_t* cmd_ptr = &msg_data[0];
    uint8_t* id_ptr = &msg_data[1];
    uint8_t* data_size_ptr = &msg_data[9];
    //uint8_t* end_ptr = &msg_data[10];

    *cmd_ptr = 'R';

    if(!slcan_number_to_hex(can_msg->id, id_ptr, 8)){
        return false;
    }

    *data_size_ptr = '0' + can_msg->data_size;

    //*end_ptr = SLCAN_EOM_BYTE;

    slcan_cmd_buf_set_size(cmd, 1 + 8 + 1 + 0 + 0 /* EOM */);

    return true;
}

bool slcan_can_msg_to_buf(slcan_can_msg_t* can_msg, slcan_cmd_buf_t* cmd)
{
    if(can_msg == NULL || cmd == NULL) return false;

    if(can_msg->frame_type == SLCAN_MSG_FRAME_TYPE_NORMAL){
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return slcan_can_msg_to_buf_t(can_msg, cmd);
        }else{ //SLCAN_MSG_ID_EXTENDED
            return slcan_can_msg_to_buf_T(can_msg, cmd);
        }
    }else{ // SLCAN_MSG_FRAME_TYPE_RTR
        if(can_msg->id_type == SLCAN_MSG_ID_NORMAL){
            return slcan_can_msg_to_buf_r(can_msg, cmd);
        }else{ //SLCAN_MSG_ID_EXTENDED
            return slcan_can_msg_to_buf_R(can_msg, cmd);
        }
    }

    return false;
}

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

ALWAYS_INLINE static void slcan_process_cmd_default(slcan_t* sc)
{
    slcan_send_answer_err(sc);
}

static void slcan_process_cmd_S(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 2 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    uint8_t can_baud_code = cmd_data[0];

    if(can_baud_code < '0' || can_baud_code > '8'){
        slcan_send_answer_err(sc);
        return;
    }

    // not need to setup can speed.

    sc->flags |= SLCAN_FLAG_CONFIGURED;

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_s(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 5 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    uint8_t* btr0_pair = &cmd_data[0];
    uint8_t* btr1_pair = &cmd_data[2];

    uint8_t btr0, btr1;

    if(!slcan_hex_pair_to_number(btr0_pair, &btr0) ||
       !slcan_hex_pair_to_number(btr1_pair, &btr1)){
        slcan_send_answer_err(sc);
        return;
    }

    // not need to setup can speed.

    sc->flags |= SLCAN_FLAG_CONFIGURED;

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_O(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_CONFIGURED)){
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    // not need to open can.

    // set opened flag.
    sc->flags |= SLCAN_FLAG_OPENED;
    // clear listen only flag.
    sc->flags &= ~SLCAN_FLAG_LISTEN_ONLY;

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_L(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    // not need to open can.

    // set opened flag.
    sc->flags |= SLCAN_FLAG_OPENED;
    // set listen only flag.
    sc->flags |= SLCAN_FLAG_LISTEN_ONLY;

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_C(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size < 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    // not need to open can.

    // clear opened flag.
    sc->flags &= ~SLCAN_FLAG_OPENED;
    // clear listen only flag.
    //sc->flags &= ~SLCAN_FLAG_LISTEN_ONLY;

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_t(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;
    slcan_can_msg_t can_msg;

    if(!slcan_can_msg_from_buf_t(&can_msg, msg)){
        slcan_send_answer_err(sc);
        return;
    }

    if(slcan_can_fifo_put(&sc->txcanfifo, &can_msg) != 1){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_z_ok(sc);
    }else{
        slcan_send_answer_ok(sc);
    }
}

static void slcan_process_cmd_T(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;
    slcan_can_msg_t can_msg;

    if(!slcan_can_msg_from_buf_T(&can_msg, msg)){
        slcan_send_answer_err(sc);
        return;
    }

    if(slcan_can_fifo_put(&sc->txcanfifo, &can_msg) != 1){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_z_ok(sc);
    }else{
        slcan_send_answer_ok(sc);
    }
}

static void slcan_process_cmd_r(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;
    slcan_can_msg_t can_msg;

    if(!slcan_can_msg_from_buf_r(&can_msg, msg)){
        slcan_send_answer_err(sc);
        return;
    }

    if(slcan_can_fifo_put(&sc->txcanfifo, &can_msg) != 1){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_z_ok(sc);
    }else{
        slcan_send_answer_ok(sc);
    }
}

static void slcan_process_cmd_R(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;
    slcan_can_msg_t can_msg;

    if(!slcan_can_msg_from_buf_R(&can_msg, msg)){
        slcan_send_answer_err(sc);
        return;
    }

    if(slcan_can_fifo_put(&sc->txcanfifo, &can_msg) != 1){
        sc->errors |= SLCAN_ERROR_OVERRUN;
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_z_ok(sc);
    }else{
        slcan_send_answer_ok(sc);
    }
}

static void slcan_process_cmd_P(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    slcan_send_next_msg(sc);
}

static void slcan_process_cmd_A(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    if(sc->flags & SLCAN_FLAG_AUTO_POLL){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    slcan_send_all_msgs(sc);

    slcan_send_answer_a_ok(sc);
}

static void slcan_process_cmd_F(slcan_t* sc)
{
    assert(sc != NULL);

    if(!(sc->flags & SLCAN_FLAG_OPENED)){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    slcan_cmd_buf_t* answer_msg = &sc->txcmd;
    slcan_cmd_buf_reset(answer_msg);

    uint32_t status = 0;

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

    uint8_t* answer_data = slcan_cmd_buf_data(answer_msg);
    answer_data[0] = 'F';
    if(!slcan_number_to_hex(status, &answer_data[1], 2)){
        slcan_send_answer_err(sc);
        return;
    }
    answer_data[3] = SLCAN_EOM_BYTE;
    slcan_cmd_buf_set_size(answer_msg, 4);

    if(!slcan_io_fifo_write_block(&sc->txiofifo,
                                  slcan_cmd_buf_data(answer_msg),
                                  slcan_cmd_buf_size(answer_msg))){
        slcan_send_answer_err(sc);
        return;
    }
}

static void slcan_process_cmd_X(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 2 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    uint8_t autopoll_enable_code = cmd_data[0];

    if(autopoll_enable_code < '0' || autopoll_enable_code > '1'){
        slcan_send_answer_err(sc);
        return;
    }

    uint32_t autopoll_enable = autopoll_enable_code - '0';

    if(autopoll_enable == 0){
        sc->flags &= ~SLCAN_FLAG_AUTO_POLL;
    }else{ //1
        sc->flags |= SLCAN_FLAG_AUTO_POLL;
    }

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_W(slcan_t* sc)
{
    slcan_send_answer_err(sc);
}

static void slcan_process_cmd_M(slcan_t* sc)
{
    slcan_send_answer_err(sc);
}

static void slcan_process_cmd_m(slcan_t* sc)
{
    slcan_send_answer_err(sc);
}

static void slcan_process_cmd_U(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 2 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    uint8_t uart_baud_code = cmd_data[0];

    if(uart_baud_code < '0' || uart_baud_code > '6'){
        slcan_send_answer_err(sc);
        return;
    }

    //slcan_port_baud_t uart_baud = uart_baud_code - '0';

    // set slcan saved port baud property.
    // slcan_set_saved_port_baud(...);

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_V(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    slcan_cmd_buf_t* answer_msg = &sc->txcmd;
    slcan_cmd_buf_reset(answer_msg);

    uint8_t* answer_data = slcan_cmd_buf_data(answer_msg);
    answer_data[0] = 'V';
    if(!slcan_number_to_hex(SLCAN_VERSION, &answer_data[1], 4)){
        slcan_send_answer_err(sc);
        return;
    }
    answer_data[5] = SLCAN_EOM_BYTE;
    slcan_cmd_buf_set_size(answer_msg, 6);

    if(!slcan_io_fifo_write_block(&sc->txiofifo,
                                  slcan_cmd_buf_data(answer_msg),
                                  slcan_cmd_buf_size(answer_msg))){
        slcan_send_answer_err(sc);
        return;
    }
}

static void slcan_process_cmd_N(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    //uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 1 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    //uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.
    slcan_cmd_buf_t* answer_msg = &sc->txcmd;
    slcan_cmd_buf_reset(answer_msg);

    uint8_t* answer_data = slcan_cmd_buf_data(answer_msg);
    answer_data[0] = 'N';
    if(!slcan_number_to_hex(SLCAN_SERIAL_NUMBER, &answer_data[1], 4)){
        slcan_send_answer_err(sc);
        return;
    }
    answer_data[5] = SLCAN_EOM_BYTE;
    slcan_cmd_buf_set_size(answer_msg, 6);

    if(!slcan_io_fifo_write_block(&sc->txiofifo,
                                  slcan_cmd_buf_data(answer_msg),
                                  slcan_cmd_buf_size(answer_msg))){
        slcan_send_answer_err(sc);
        return;
    }
}

static void slcan_process_cmd_Z(slcan_t* sc)
{
    assert(sc != NULL);

    if(sc->flags & SLCAN_FLAG_OPENED){
        slcan_send_answer_err(sc);
        return;
    }

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    size_t msg_data_size = slcan_cmd_buf_size(msg);

    if(msg_data_size != 2 + 0 /* EOM */){
        slcan_send_answer_err(sc);
        return;
    }

    //uint8_t cmd = msg_data[0];
    // if(cmd != ''){
    //     slcan_send_answer_err(sc);
    //     return;
    // }

    uint8_t* cmd_data = &msg_data[1];
    //size_t cmd_data_size = msg_data_size - 1;

    // process cmd.

    uint8_t timestamp_enable_code = cmd_data[0];

    if(timestamp_enable_code < '0' || timestamp_enable_code > '1'){
        slcan_send_answer_err(sc);
        return;
    }

    uint32_t timestamp_enable = timestamp_enable_code - '0';

    if(timestamp_enable == 0){
        sc->flags &= ~SLCAN_FLAG_TIMESTAMP;
    }else{ //1
        sc->flags |= SLCAN_FLAG_TIMESTAMP;
    }

    slcan_send_answer_ok(sc);
}

static void slcan_process_cmd_Q(slcan_t* sc)
{
    slcan_send_answer_err(sc);
}

static void slcan_process_incoming_msg(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_cmd_buf_t* msg = &sc->rxcmd;

    uint8_t* msg_data = slcan_cmd_buf_data(msg);
    //size_t msg_data_size = slcan_cmd_buf_size(msg);

    uint8_t cmd = msg_data[0];

#if defined(SLCAN_DEBUG_INCOMING_MSGS) && SLCAN_DEBUG_INCOMING_MSGS == 1
    msg_data[slcan_cmd_buf_size(msg)] = '\0';
    printf("Received msg: %s\n", (char*)msg_data);
#endif

    switch(cmd){
        default:
            slcan_process_cmd_default(sc);
            break;
        case 'S':
            slcan_process_cmd_S(sc);
            break;
        case 's':
            slcan_process_cmd_s(sc);
            break;
        case 'O':
            slcan_process_cmd_O(sc);
            break;
        case 'L':
            slcan_process_cmd_L(sc);
            break;
        case 'C':
            slcan_process_cmd_C(sc);
            break;
        case 't':
            slcan_process_cmd_t(sc);
            break;
        case 'T':
            slcan_process_cmd_T(sc);
            break;
        case 'r':
            slcan_process_cmd_r(sc);
            break;
        case 'R':
            slcan_process_cmd_R(sc);
            break;
        case 'P':
            slcan_process_cmd_P(sc);
            break;
        case 'A':
            slcan_process_cmd_A(sc);
            break;
        case 'F':
            slcan_process_cmd_F(sc);
            break;
        case 'X':
            slcan_process_cmd_X(sc);
            break;
        case 'W':
            slcan_process_cmd_W(sc);
            break;
        case 'M':
            slcan_process_cmd_M(sc);
            break;
        case 'm':
            slcan_process_cmd_m(sc);
            break;
        case 'U':
            slcan_process_cmd_U(sc);
            break;
        case 'V':
            slcan_process_cmd_V(sc);
            break;
        case 'N':
            slcan_process_cmd_N(sc);
            break;
        case 'Z':
            slcan_process_cmd_Z(sc);
            break;
        case 'Q':
            slcan_process_cmd_Q(sc);
            break;
    }
}


int slcan_init(slcan_t* sc, slcan_serial_io_t* sio, slcan_get_timestamp_t get_timestamp)
{
    assert(sc != NULL);

    if(sio == NULL) return -1;

    sc->sio = sio;
    sc->get_timestamp = get_timestamp;

    slcan_get_default_port_config(&sc->port_conf);

    sc->serial_port = -1;

    slcan_io_fifo_init(&sc->txiofifo);
    slcan_io_fifo_init(&sc->rxiofifo);
    slcan_cmd_buf_init(&sc->txcmd);
    slcan_cmd_buf_init(&sc->rxcmd);
    slcan_can_fifo_init(&sc->txcanfifo);
    slcan_can_fifo_init(&sc->rxcanfifo);

    sc->errors = 0;

    sc->flags = 0;

#if defined(SLCAN_AUTO_POLL_DEFAULT)
#if SLCAN_AUTO_POLL_DEFAULT == 1
    sc->flags |= SLCAN_FLAG_AUTO_POLL;
#endif
#endif

#if defined(SLCAN_TIMESTAMP_DEFAULT)
#if SLCAN_TIMESTAMP_DEFAULT == 1
    sc->flags |= SLCAN_FLAG_TIMESTAMP;
#endif
#endif

    return 0;
}

int slcan_open(slcan_t* sc, const char* serial_port_name)
{
    assert(sc != NULL);

    int res = slcan_serial_open(sc, serial_port_name);
    if(res == -1) return res;

    sc->serial_port = res;

    return 0;
}

int slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf)
{
    assert(sc != NULL);

    if(port_conf == NULL) port_conf = &sc->port_conf;

    int res = slcan_serial_configure(sc, port_conf);
    if(res == -1) return res;

    sc->port_conf.parity = port_conf->parity;
    sc->port_conf.stop_bits = port_conf->stop_bits;
    sc->port_conf.baud = port_conf->baud;

    return 0;
}

void slcan_deinit(slcan_t* sc)
{
    assert(sc != NULL);

    slcan_serial_close(sc);
}

int slcan_poll(slcan_t* sc)
{
    assert(sc != NULL);

    int res;
    size_t size;
    uint8_t byte;

    // process data in can msg tx fifo.
    if(sc->flags & SLCAN_FLAG_OPENED){
        if(sc->flags & SLCAN_FLAG_AUTO_POLL){
            slcan_send_all_msgs(sc);
        }
    }

    // poll.
    int revents = 0;
    res = slcan_serial_poll(sc, SLCAN_POLLIN | SLCAN_POLLOUT, &revents, 0);
    if(res == -1){
        sc->errors |= SLCAN_ERROR_IO;
    }

    // incoming data.
    if(revents & SLCAN_POLLIN){
        slcan_process_incoming_data(sc);
    }

    // outcoming data.
    if(revents & SLCAN_POLLOUT){
        slcan_process_outcoming_data(sc);
    }


    // process data rx fifo.
    for(;;){
        // get byte.
        size = slcan_io_fifo_get(&sc->rxiofifo, &byte);
        // fifo is empty.
        if(size == 0){
            break;
        }
        // end of msg.
        if(byte == SLCAN_EOM_BYTE){
            // parse msg.
            slcan_process_incoming_msg(sc);
            // reset processed msg.
            slcan_cmd_buf_reset(&sc->rxcmd);
        }else{
            // put data to msg buf.
            size = slcan_cmd_buf_put(&sc->rxcmd, byte);
            // msg buf is full.
            if(size == 0){
                // set overrun flag.
                sc->errors |= SLCAN_ERROR_OVERRUN;
                // reset received data.
                slcan_cmd_buf_reset(&sc->rxcmd);
                break;
            }
        }
    }
    return 0;
}

size_t slcan_get_can_message(slcan_t* sc, slcan_can_msg_t* can_msg)
{
    return slcan_can_fifo_get(&sc->txcanfifo, can_msg);
}

size_t slcan_put_can_message(slcan_t* sc, const slcan_can_msg_t* can_msg)
{
    return slcan_can_fifo_put(&sc->rxcanfifo, can_msg);
}
