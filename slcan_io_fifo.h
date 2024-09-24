#ifndef SLCAN_IO_FIFO_H_
#define SLCAN_IO_FIFO_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_conf.h"


//! Количество данных.
#ifndef SLCAN_IO_FIFO_SIZE
#define SLCAN_IO_FIFO_SIZE SLCAN_IO_FIFO_DEFAULT_SIZE
#endif


//! Тип фифо.
typedef struct _Slcan_Io_Fifo {
    uint8_t buf[SLCAN_IO_FIFO_SIZE]; //!< Данные.
    size_t wptr; //!< Индекс для записи.
    size_t rptr; //!< Индекс для чтения.
    size_t count; //!< Количество данных.
} slcan_io_fifo_t;


/**
 * Инициализирует фифо.
 * @param fifo Фифо.
 */
EXTERN void slcan_io_fifo_init(slcan_io_fifo_t* fifo);

/**
 * Сбрасывает фифо.
 * @param fifo Фифо.
 */
ALWAYS_INLINE static void slcan_io_fifo_reset(slcan_io_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

/**
 * Получает указатель на данные для чтения.
 * @param fifo Фифо.
 * @return Указатель на данные для чтения.
 */
ALWAYS_INLINE static uint8_t* slcan_io_fifo_data_to_read(slcan_io_fifo_t* fifo)
{
    return &fifo->buf[fifo->rptr];
}

/**
 * Получает указатель на данные для записи.
 * @param fifo Фифо.
 * @return Указатель на данные для записи.
 */
ALWAYS_INLINE static uint8_t* slcan_io_fifo_data_to_write(slcan_io_fifo_t* fifo)
{
    return &fifo->buf[fifo->wptr];
}

/**
 * Получает количество доступных для чтения данных.
 * @param fifo Фифо.
 * @return Количество доступных для чтения данных.
 */
ALWAYS_INLINE static size_t slcan_io_fifo_avail(const slcan_io_fifo_t* fifo)
{
    return fifo->count;
}

/**
 * Получает оставшееся место для записи данных.
 * @param fifo Фифо.
 * @return Размер данных, которые могут быть записаны.
 */
ALWAYS_INLINE static size_t slcan_io_fifo_remain(const slcan_io_fifo_t* fifo)
{
    return SLCAN_IO_FIFO_SIZE - fifo->count;
}

/**
 * Получает флаг заполненности фифо.
 * @param fifo Фифо.
 * @return Флаг заполненности фифо.
 */
ALWAYS_INLINE static bool slcan_io_fifo_full(const slcan_io_fifo_t* fifo)
{
    return fifo->count == SLCAN_IO_FIFO_SIZE;
}

/**
 * Получает флаг пустоты фифо.
 * @param fifo Фифо.
 * @return Флаг пустоты фифо.
 */
ALWAYS_INLINE static bool slcan_io_fifo_empty(const slcan_io_fifo_t* fifo)
{
    return fifo->count == 0;
}

/**
 * Получает размер данных для линейного чтения из фифо.
 * @param fifo Фифо.
 * @return Размер данных для линейного чтения из фифо.
 */
EXTERN size_t slcan_io_fifo_read_line_size(const slcan_io_fifo_t* fifo);

/**
 * Получает размер данных для линейной записи в фифо.
 * @param fifo Фифо.
 * @return Размер данных для линейной записи в фифо.
 */
EXTERN size_t slcan_io_fifo_write_line_size(const slcan_io_fifo_t* fifo);

/**
 * Помещает данные в фифо.
 * @param fifo Фифо.
 * @param data Данные.
 * @return Количество помещённых данных, 0 - при невозможности поместить данные (фифо полное).
 */
EXTERN size_t slcan_io_fifo_put(slcan_io_fifo_t* fifo, uint8_t data);

/**
 * Получает данные из фифо.
 * @param fifo Фифо.
 * @param data Указатель для получения данных.
 * @return Количество полученных данных, 0 - при невозможности получить данные (фифо пустое).
 */
EXTERN size_t slcan_io_fifo_get(slcan_io_fifo_t* fifo, uint8_t* data);

/**
 * Получает данные из фифо, не убирая их.
 * @param fifo Фифо.
 * @param data Указатель для получения данных.
 * @return Количество полученных данных, 0 - при невозможности получить данные (фифо пустое).
 */
EXTERN size_t slcan_io_fifo_peek(const slcan_io_fifo_t* fifo, uint8_t* data);

/**
 * Записывает данные в фифо.
 * @param fifo Фифо.
 * @param data Данные.
 * @param data_size Размер данных.
 * @return Количество помещённых данных, 0 - при невозможности поместить данные (фифо полное).
 */
EXTERN size_t slcan_io_fifo_write(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size);

/**
 * Помещает атомарно блок данных в фифо.
 * @param fifo Фифо.
 * @param data Данные.
 * @param data_size Размер данных.
 * @return Флаг помещения данных в фифо.
 */
EXTERN bool slcan_io_fifo_write_block(slcan_io_fifo_t* fifo, const uint8_t* data, size_t data_size);

/**
 * Читает данные из фифо.
 * @param fifo Фифо.
 * @param data Указатель для получения данных.
 * @param data_size Размер данных.
 * @return Количество полученных данных, 0 - при невозможности получить данные (фифо пустое).
 */
EXTERN size_t slcan_io_fifo_read(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size);

/**
 * Читает атомарно блок данных из фифо.
 * @param fifo Фифо.
 * @param data Указатель для получения данных.
 * @param data_size Размер данных.
 * @return Флаг чтения данных из фифо.
 */
EXTERN bool slcan_io_fifo_read_block(slcan_io_fifo_t* fifo, uint8_t* data, size_t data_size);

/**
 * Оповещает фифо о чтении заданного размера данных.
 * @param fifo Фифо.
 * @param data_size Размер данных.
 */
EXTERN void slcan_io_fifo_data_readed(slcan_io_fifo_t* fifo, size_t data_size);

/**
 * Оповещает фифо о записи заданного размера данных.
 * @param fifo Фифо.
 * @param data_size Размер данных.
 */
EXTERN void slcan_io_fifo_data_written(slcan_io_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_IO_FIFO_H_ */
