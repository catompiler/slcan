#ifndef UTILS_SLCAN_SERIAL_IO_H_
#define UTILS_SLCAN_SERIAL_IO_H_

#include <stddef.h>


typedef enum _Slcan_Port_Parity {
    SLCAN_PORT_PARITY_NONE = 0,
    SLCAN_PORT_PARITY_EVEN = 1,
    SLCAN_PORT_PARITY_ODD = 2,
} slcan_port_parity_t;

typedef enum _Slcan_Port_Baud {
    SLCAN_PORT_BAUD_230400 = 0, //!< 230400
    SLCAN_PORT_BAUD_115200 = 1, //!< 115200
    SLCAN_PORT_BAUD_57600 = 2, //!< 57600
    SLCAN_PORT_BAUD_38400 = 3, //!< 38400
    SLCAN_PORT_BAUD_19200 = 4, //!< 19200
    SLCAN_PORT_BAUD_9600 = 5, //!< 9600
    SLCAN_PORT_BAUD_2400 = 6, //!< 2400
} slcan_port_baud_t;

typedef enum _Slcan_Port_Stop_Bits {
    SLCAN_PORT_STOP_BITS_1 = 0,
    SLCAN_PORT_STOP_BITS_2 = 1,
} slcan_port_stop_bits_t;

typedef struct _Slcan_Port_Conf {
    slcan_port_parity_t parity;
    slcan_port_baud_t baud;
    slcan_port_stop_bits_t stop_bits;
} slcan_port_conf_t;

typedef enum _Slcan_Poll {
    SLCAN_POLLIN = 1,
    SLCAN_POLLOUT = 4
} slcan_poll_t;


/*
 * -1 for error, 0 or non-negative for success.
 */
typedef int (*slcan_serial_open_t)(const char* serial_port_name);
typedef int (*slcan_serial_configure_t)(int serial_port, slcan_port_conf_t* conf);
typedef void (*slcan_serial_close_t)(int serial_port);
typedef int (*slcan_serial_read_t)(int serial_port, void* data, size_t data_size);
typedef int (*slcan_serial_write_t)(int serial_port, const void* data, size_t data_size);
typedef int (*slcan_serial_flush_t)(int serial_port);
typedef int (*slcan_serial_poll_t)(int serial_port, int events, int* revents, int timeout);
typedef int (*slcan_serial_nbytes_t)(int serial_port, size_t* size);
//typedef int (*_t)();


typedef struct _Slcan_Serial_Io {
    slcan_serial_open_t open;
    slcan_serial_configure_t configure;
    slcan_serial_close_t close;
    slcan_serial_read_t read;
    slcan_serial_write_t write;
    slcan_serial_flush_t flush;
    slcan_serial_poll_t poll;
    slcan_serial_nbytes_t nbytes;
} slcan_serial_io_t;


#endif /* UTILS_SLCAN_SERIAL_IO_H_ */
