#ifndef SLCAN_SLAVE_H_
#define SLCAN_SLAVE_H_

#include "slcan.h"
#include "slcan_cmd.h"
#include "slcan_serial_io.h"
#include "slcan_can_fifo.h"
#include "slcan_can_ext_fifo.h"


// Тип структуры будущего.
typedef struct _Slcan_Future slcan_future_t;

// Структура отметки времени.
struct timespec;


//! Тип коллбэка настройки CAN на стандартную скорость.
typedef slcan_err_t (*slcan_on_setup_can_std_t)(slcan_bit_rate_t bit_rate);
//! Тип коллбэка настройки CAN а нестандартную скорость.
typedef slcan_err_t (*slcan_on_setup_can_btr_t)(uint8_t btr0, uint8_t btr1);
//! Тип коллбэка открытия CAN.
typedef slcan_err_t (*slcan_on_open_t)(void);
//! Тип коллбэка открытия CAN на прослушку.
typedef slcan_err_t (*slcan_on_listen_t)(void);
//! Тип коллбэка закрытия CAN.
typedef slcan_err_t (*slcan_on_close_t)(void);
//! Тип коллбэка настройки UART.
typedef slcan_err_t (*slcan_on_setup_uart_t)(slcan_port_baud_t baud);
//! Тип коллбэка установки маски фильтра CAN.
typedef slcan_err_t (*slcan_on_set_acceptance_mask_t)(uint32_t value);
//! Тип коллбэка установки значения фильтра CAN.
typedef slcan_err_t (*slcan_on_set_acceptance_filter_t)(uint32_t value);


//! Структура коллбэков.
typedef struct _Slcan_Slave_Callbacks {
    slcan_on_setup_can_std_t on_setup_can_std;
    slcan_on_setup_can_btr_t on_setup_can_btr;
    slcan_on_open_t on_open;
    slcan_on_listen_t on_listen;
    slcan_on_close_t on_close;
    slcan_on_setup_uart_t on_setup_uart;
    slcan_on_set_acceptance_mask_t on_set_acceptance_mask;
    slcan_on_set_acceptance_filter_t on_set_acceptance_filter;
} slcan_slave_callbacks_t;


//! Перечисление флагов ведомого устройства.
typedef enum _Slcan_Slave_Flag {
    SLCAN_SLAVE_FLAG_NONE = 0x0,
    SLCAN_SLAVE_FLAG_CONFIGURED = (1<<0),
    SLCAN_SLAVE_FLAG_OPENED = (1<<1),
    SLCAN_SLAVE_FLAG_LISTEN_ONLY = (1<<2),
    SLCAN_SLAVE_FLAG_AUTO_POLL = (1<<3),
    SLCAN_SLAVE_FLAG_TIMESTAMP = (1<<4),
} slcan_slave_flag_t;

//! Тип флагов ведомого устройства.
typedef uint32_t slcan_slave_flags_t;

//! Перечисление ошибок ведомого устройства.
typedef enum _Slcan_Slave_Error {
    SLCAN_SLAVE_ERROR_NONE = 0x0,
    SLCAN_SLAVE_ERROR_IO = 0x1,
    SLCAN_SLAVE_ERROR_OVERRUN = 0x2,
    SLCAN_SLAVE_ERROR_ARBITRATION_LOST = 0x4,
} slcan_slave_error_t;

//! Тип ошибок ведомого устройства.
typedef uint32_t slcan_slave_errors_t;



//! Структура ведомого устройства.
typedef struct _Slcan_Slave {
    slcan_t* sc; //!< Последовательный интерфейс.
    slcan_slave_callbacks_t* cb; //!< Коллбэки функций.
    slcan_can_ext_fifo_t rxcanfifo; //!< Фифо полученных сообщений CAN.
    slcan_can_fifo_t txcanfifo; //!< Фифо передаваемых сообщений CAN.
    slcan_slave_flags_t flags; //!< Флаги.
    slcan_slave_errors_t errors; //!< Ошибки.
} slcan_slave_t;

/**
 * Инициализирует ведомое устройство.
 * @param scs Ведомое устройство.
 * @param sc Последовательный интерфейс.
 * @param cb Коллбэки.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_slave_init(slcan_slave_t* scs, slcan_t* sc, slcan_slave_callbacks_t* cb);

/**
 * Деинициализирует ведомое устройство.
 * @param scs Ведомое устройство.
 */
EXTERN void slcan_slave_deinit(slcan_slave_t* scs);

/**
 * Получает последовательный интерфейс.
 * @param scs Ведомое устройство.
 * @return Последовательный интерфейс.
 */
ALWAYS_INLINE static slcan_t* slcan_slave_slcan(slcan_slave_t* scs)
{
    return scs->sc;
}

/**
 * Обрабатывает события ведомого устройство.
 * @param scs Ведомое устройство.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_slave_poll(slcan_slave_t* scs);

/**
 * Ждёт не более чем заданный тайм-аут
 * отправки полученных фреймов.
 * @param scs Ведомое устройство.
 * @param tp_timeout Тайм-аут.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_slave_slave_flush(slcan_slave_t* scs, struct timespec* tp_timeout);

/**
 * Отправляет сообщение CAN.
 * @param scs Ведомое устройство.
 * @param can_msg Сообщение CAN.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_slave_send_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg, slcan_future_t* future);

/**
 * Получает сообщение CAN.
 * @param scs Ведомое устройство.
 * @param can_msg Сообщение CAN.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_slave_recv_can_msg(slcan_slave_t* scs, slcan_can_msg_t* can_msg);

/**
 * Получает флаги ведомого устройства.
 * @param scs Ведомое устройство.
 * @return Флаги ведомого устройства.
 */
ALWAYS_INLINE slcan_slave_flags_t slcan_slave_flags(slcan_slave_t* scs)
{
    return scs->flags;
}

/**
 * Устанавливает флаги ведомого устройства.
 * @param scs Ведомое устройства.
 * @param flags Флаги.
 */
ALWAYS_INLINE void slcan_slave_set_flags(slcan_slave_t* scs, slcan_slave_flags_t flags)
{
    scs->flags = flags;
}

/**
 * Получает ошибки ведомого устройства.
 * @param scs Ведомое устройство.
 * @return Ошибки ведомого устройства.
 */
ALWAYS_INLINE slcan_slave_errors_t slcan_slave_errors(slcan_slave_t* scs)
{
    return scs->errors;
}

/**
 * Устанавливает ошибки ведомого устройства.
 * @param scs Ведомое устройства.
 * @param errors Ошибки.
 */
ALWAYS_INLINE void slcan_slave_set_errors(slcan_slave_t* scs, slcan_slave_errors_t errors)
{
    scs->errors = errors;
}

#endif /* SLCAN_SLAVE_H_ */
