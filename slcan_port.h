#ifndef SLCAN_PORT_H_
#define SLCAN_PORT_H_


#include <time.h>
#include "slcan_defs.h"
#include "slcan_serial_io.h"


/**
 * Получает текущую отметку времени.
 * @param clock_id Часы.
 * @param tp Отметка времени.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_clock_gettime (clockid_t clock_id, struct timespec *tp);


/**
 * Открывает последовательный порт.
 * @param serial_port_name Имя последовательного порта.
 * @param serial_port Возвращаемый идентификатор последовательного порта.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_open(const char* serial_port_name, slcan_serial_handle_t* serial_port);

/**
 * Настраивает последовательный порт.
 * @param serial_port Идентификатор последовательного порта.
 * @param conf Конфигурация последовательного порта.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_configure(slcan_serial_handle_t serial_port, const slcan_port_conf_t* conf);

/**
 * Закрывает последовательный порт.
 * @param serial_port Идентификатор последовательного порта.
 */
EXTERN void slcan_serial_close(slcan_serial_handle_t serial_port);

/**
 * Читает данные.
 * @param serial_port Идентификатор последовательного порта.
 * @param data Буфер для данных.
 * @param data_size Максимальный размер читаемых данных.
 * @return Число прочитанных байт в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_read(slcan_serial_handle_t serial_port, void* data, size_t data_size);

/**
 * Записывает данные.
 * @param serial_port Идентификатор последовательного порта.
 * @param data Данные.
 * @param data_size Размер данных.
 * @return Число записанных байт в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_write(slcan_serial_handle_t serial_port, const void* data, size_t data_size);

/**
 * Ждёт окончания передачи.
 * @param serial_port Идентификатор последовательного порта.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_flush(slcan_serial_handle_t serial_port);

/**
 * Проверяет возникшине события.
 * @param serial_port Идентификатор последовательного порта.
 * @param events События для проверки.
 * @param revents Возвращаемые события.
 * @param timeout Тайм-аут.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_poll(slcan_serial_handle_t serial_port, int events, int* revents, int timeout);

/**
 * Получает доступное для чтения число байт.
 * @param serial_port Идентификатор последовательного порта.
 * @param size Возвращаемое значение числа доступных байт.
 * @return SLCAN_IO_SUCCESS в случае успеха, иначе SLCAN_IO_FAIL.
 */
EXTERN int slcan_serial_nbytes(slcan_serial_handle_t serial_port, size_t* size);


#endif /* SLCAN_PORT_H_ */
