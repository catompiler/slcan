#ifndef SLCAN_SERIAL_IO_H_
#define SLCAN_SERIAL_IO_H_

#include <stddef.h>


//! Тип идентификатора ввода-вывода последовательного порта.
typedef void* slcan_serial_handle_t;

//! Значение недействительного идентификатора ввода-вывода последовательного порта.
#define SLCAN_IO_INVALID_HANDLE ((slcan_serial_handle_t)(long)(-1))


//! Значение, возвращаемое при неудачном вызове.
#define SLCAN_IO_FAIL (-1)

//! Значение, возвращаемое при успешном вызове.
#define SLCAN_IO_SUCCESS (0)


//! Перечисление значений чётности последовательного порта.
typedef enum _Slcan_Port_Parity {
    SLCAN_PORT_PARITY_NONE = 0,
    SLCAN_PORT_PARITY_EVEN = 1,
    SLCAN_PORT_PARITY_ODD = 2,
} slcan_port_parity_t;

//! Перечисление значений скорости последовательного порта.
typedef enum _Slcan_Port_Baud {
    SLCAN_PORT_BAUD_230400 = 0, //!< 230400
    SLCAN_PORT_BAUD_115200 = 1, //!< 115200
    SLCAN_PORT_BAUD_57600 = 2, //!< 57600
    SLCAN_PORT_BAUD_38400 = 3, //!< 38400
    SLCAN_PORT_BAUD_19200 = 4, //!< 19200
    SLCAN_PORT_BAUD_9600 = 5, //!< 9600
    SLCAN_PORT_BAUD_2400 = 6, //!< 2400
} slcan_port_baud_t;

//! Перечисление числа стоповых бит порта.
typedef enum _Slcan_Port_Stop_Bits {
    SLCAN_PORT_STOP_BITS_1 = 0,
    SLCAN_PORT_STOP_BITS_2 = 1,
} slcan_port_stop_bits_t;

//! Структура конфигурации порта.
typedef struct _Slcan_Port_Conf {
    slcan_port_parity_t parity; //!< Чётность.
    slcan_port_baud_t baud; //!< Скорость.
    slcan_port_stop_bits_t stop_bits; //!< Стоповые биты.
} slcan_port_conf_t;

//! Перечисление флагов для poll (events & revents).
typedef enum _Slcan_Poll {
    SLCAN_POLLIN = 1, //!< Входящие данные.
    SLCAN_POLLOUT = 4 //!< Готовность передавать новые данные.
} slcan_poll_t;


#endif /* SLCAN_SERIAL_IO_H_ */
