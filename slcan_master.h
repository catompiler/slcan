#ifndef SLCAN_MASTER_H_
#define SLCAN_MASTER_H_

#include "slcan.h"
#include "slcan_resp_out_fifo.h"
#include "slcan_can_fifo.h"
#include "slcan_can_ext_fifo.h"
#include "slcan_slave_status.h"
#include "slcan_conf.h"


// Тип структуры будущего.
typedef struct _Slcan_Future slcan_future_t;

// Структура отметки времени.
struct timespec;


//! Структура ведущего устройства.
typedef struct _Slcan_Master {
    slcan_t* sc; //!< Последовательный интерфейс.
    slcan_resp_out_fifo_t respoutfifo; //!< Фифо запросов.
    slcan_can_ext_fifo_t rxcanfifo; //!< Фифо полученных сообщений CAN.
    slcan_can_fifo_t txcanfifo; //!< Фифо передаваемых сообщений CAN.
    struct timespec tp_timeout; //!< Тайм-аут запросов.
} slcan_master_t;

/**
 * Инициализирует ведущее устройство.
 * @param scm Ведущее устройство.
 * @param sc Последовательный интерфейс.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_init(slcan_master_t* scm, slcan_t* sc);

/**
 * Деинициализирует ведущее устройство.
 * @param scm Ведущее устройство.
 */
EXTERN void slcan_master_deinit(slcan_master_t* scm);

/**
 * Получает последовательный интерфейс.
 * @param scm Ведущее устройство.
 * @return Последовательный интерфейс.
 */
ALWAYS_INLINE static slcan_t* slcan_master_slcan(slcan_master_t* scm)
{
    return scm->sc;
}

/**
 * Получает число принятых сообщений CAN.
 * @param scm Ведущее устройство.
 * @return Число принятых сообщений CAN.
 */
EXTERN size_t slcan_master_received_can_msgs_count(slcan_master_t* scm);

/**
 * Получает число свободных мест для передачи сообщений CAN.
 * @param scm Ведущее устройство.
 * @return Число свободных мест для передачи сообщений CAN.
 */
EXTERN size_t slcan_master_send_can_msgs_avail(slcan_master_t* scm);

/**
 * Устанавливает тайм-аут запросов.
 * @param scm Ведущее устройство.
 * @param tp_timeout Тайм-аут.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_set_timeout(slcan_master_t* scm, const struct timespec* tp_timeout);

/**
 * Обрабатывает события ведущего устройства.
 * @param scm Ведущее устройство.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_poll(slcan_master_t* scm);

/**
 * Ждёт не более чем заданный тайм-аут
 * завершения имеющихся запросов.
 * @param scm Ведущее устройство.
 * @param tp_timeout Тайм-аут.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_flush(slcan_master_t* scm, struct timespec* tp_timeout);

/**
 * Сбрасывает ведущее устройство.
 * @param scm Ведущее устройство.
 */
EXTERN void slcan_master_reset(slcan_master_t* scm);

/**
 * Отправляет запрос настройки CAN на стандартную скорость.
 * @param scm Ведущее устройство.
 * @param bit_rate Скорость.
 * @param future Будущее. Может быть NULL.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_setup_can_std(slcan_master_t* scm, slcan_bit_rate_t bit_rate, slcan_future_t* future);

/**
 * Отправляет запрос настройки CAN на стандартную скорость.
 * @param scm Ведущее устройство.
 * @param btr0 Регистр BTR0.
 * @param btr1 Регистр BTR1.
 * @param future Будущее. Может быть NULL.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_setup_can_btr(slcan_master_t* scm, uint16_t btr0, uint16_t btr1, slcan_future_t* future);

/**
 * Отправляет запрос на открытие CAN.
 * @param scm Ведущее устройство.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_open(slcan_master_t* scm, slcan_future_t* future);

/**
 * Отправляет запрос на открытие CAN на прослушку.
 * @param scm Ведущее устройство.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_listen(slcan_master_t* scm, slcan_future_t* future);

/**
 * Отправляет запрос на закрытие CAN.
 * @param scm Ведущее устройство.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_close(slcan_master_t* scm, slcan_future_t* future);

/**
 * Отправляет запрос на получение принятого сообщения.
 * @param scm Ведущее устройство.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_poll(slcan_master_t* scm, slcan_future_t* future);

/**
 * Отправляет запрос на получение всех принятых сообщений CAN.
 * @param scm Ведущее устройство.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_poll_all(slcan_master_t* scm, slcan_future_t* future);

/**
 * Отправляет запрос на получение статуса.
 * @param scm Ведущее устройство.
 * @param status Указатель для получения статуса.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_read_status(slcan_master_t* scm, slcan_slave_status_t* status, slcan_future_t* future);

/**
 * Отправляет запрос на установку автоматического получения принятых сообщений CAN.
 * @param scm Ведущее устройство.
 * @param enable Включение автоматического получения принятых сообщений.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_set_auto_poll(slcan_master_t* scm, bool enable, slcan_future_t* future);

/**
 * Отправляет запрос на настройку UART/
 * @param scm Ведущее устройство.
 * @param baud Скорость.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_setup_uart(slcan_master_t* scm, slcan_port_baud_t baud, slcan_future_t* future);

/**
 * Отправляет запрос на получение версии.
 * @param scm Ведущее устройство.
 * @param hw_version Указатель на версию аппаратной части.
 * @param sw_version Указатель на версию программной части.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_read_version(slcan_master_t* scm, uint8_t* hw_version, uint8_t* sw_version, slcan_future_t* future);

/**
 * Отправляет запрос на получение серийного номера.
 * @param scm Ведущее устройство.
 * @param sn Указатель для серийного номера.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_read_sn(slcan_master_t* scm, uint16_t* sn, slcan_future_t* future);

/**
 * Отправляет запрос на установку получения отметок времени,
 * @param scm Ведущее устройство.
 * @param enable Флаг получения отметок времени.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_set_timestamp(slcan_master_t* scm, bool enable, slcan_future_t* future);

/**
 * Отправляет запрос на установку маски фильтра,
 * @param scm Ведущее устройство.
 * @param value Маска.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_set_acceptance_mask(slcan_master_t* scm, uint32_t value, slcan_future_t* future);

/**
 * Отправляет запрос на установку значения фильтра,
 * @param scm Ведущее устройство.
 * @param value Значение.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_cmd_set_acceptance_filter(slcan_master_t* scm, uint32_t value, slcan_future_t* future);

/**
 * Отправляет запрос на передачу сообщения CAN.
 * @param scm Ведущее устройство.
 * @param can_msg Сообщение CAN.
 * @param future Будущее.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_send_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_future_t* future);

/**
 * Получает принятое сообщение CAN.
 * @param scm Ведущее устройство.
 * @param can_msg Сообщение CAN.
 * @param extdata Дополнительные данные сообщения CAN.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_master_recv_can_msg(slcan_master_t* scm, slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* extdata);

#endif /* SLCAN_MASTER_H_ */
