#ifndef SLCAN_H_
#define SLCAN_H_

#include "slcan_serial_io.h"
#include "slcan_io_fifo.h"
#include "slcan_cmd_buf.h"
#include "slcan_cmd.h"
#include "slcan_err.h"
#include "slcan_port.h"
#include "slcan_defs.h"
#include "slcan_conf.h"


//! Инициализация SLCAN открывает последовательный порт.
//#define SLCAN_INIT_OPENS_PORT 0

//! Деинициализация SLCAN закрывает последовательный порт.
//#define SLCAN_DEINIT_CLOSES_PORT 0


//! Порог заполнения фифо отправляемыми сообщениями.
#define SLCAN_TXIOFIFO_WATERMARK (SLCAN_IO_FIFO_SIZE / 4 * 3)


// End Of Message
#define SLCAN_EOM_BYTE '\r'
// OK
#define SLCAN_OK_BYTE '\r'
// OK if Auto Poll enabled
#define SLCAN_Z_BYTE 'Z'
// OK for poll all messages
#define SLCAN_A_BYTE 'A'
// Err
#define SLCAN_ERR_BYTE '\007'


// Структура отметки времени.
struct timespec;


//! Структура последовательного интерфейса для CAN.
typedef struct _Slcan {
    slcan_port_conf_t port_conf; //!< Конфигурация порта.
    slcan_serial_handle_t serial_port; //!< Идентификатор открытого порта.
    slcan_io_fifo_t txiofifo; //!< Фифо байт данных для передачи.
    slcan_io_fifo_t rxiofifo; //!< Фифо принятых байт данных.
    slcan_cmd_buf_t txcmd; //!< Буфер для передаваемой команды.
    slcan_cmd_buf_t rxcmd; //!< Буфер для принимаемой команды.
} slcan_t;


/**
 * Получает конфигурацию порта по-умолчанию.
 * @param conf Конфигурация порта.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_get_default_port_config(slcan_port_conf_t* conf);

/**
 * Получает текущую конфигурацию порта.
 * @param conf Конфигурация порта.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_get_port_config(slcan_t* sc, slcan_port_conf_t* conf);


/**
 * Инициализирует последовательных интерфейс CAN.
 * @param sc Интерфейс.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_init(slcan_t* sc);

/**
 * Деинициализирует последовательных интерфейс CAN.
 * @param sc Интерфейс.
 */
EXTERN void slcan_deinit(slcan_t* sc);

/**
 * Получает флаг открытого соединения.
 * @return Флаг открытого соединения.
 */
EXTERN bool slcan_opened(slcan_t* sc);

/**
 * Сбрасывает последовательный интерфейс CAN.
 * @param sc Интерфейс.
 */
EXTERN void slcan_reset(slcan_t* sc);

/**
 * Открывает порт.
 * @param sc Интерфейс.
 * @param serial_port_name
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_open(slcan_t* sc, const char* serial_port_name);

/**
 * Закрывает порт.
 * @param sc Интерфейс.
 */
EXTERN void slcan_close(slcan_t* sc);

/**
 * Настраивает порт.
 * @param sc Интерфейс.
 * @param port_conf Конфигурация порта.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_configure(slcan_t* sc, slcan_port_conf_t* port_conf);

/**
 * Обрабатывает события ввода-вывода.
 * @param sc Интерфейс.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_poll(slcan_t* sc);

/**
 * Обрабатывает события вывода.
 * @param sc Интерфейс.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_poll_out(slcan_t* sc);

/**
 * Ждёт не более чем заданный тайм-аут
 * отправки данных.
 * @param sc Интерфейс.
 * @param tp_timeout Тайм-аут.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_flush(slcan_t* sc, struct timespec* tp_timeout);

/**
 * Получает принятую команду.
 * @param sc Интерфейс.
 * @param cmd
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_get_cmd(slcan_t* sc, slcan_cmd_t* cmd);

/**
 * Отправляет команду.
 * @param sc Интерфейс.
 * @param cmd
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_put_cmd(slcan_t* sc, const slcan_cmd_t* cmd);

#endif /* SLCAN_H_ */
