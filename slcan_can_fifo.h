#ifndef SLCAN_CAN_FIFO_H_
#define SLCAN_CAN_FIFO_H_

#include <stddef.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_can_msg.h"
#include "slcan_future.h"
#include "slcan_conf.h"


//! Количество данных.
#ifndef SLCAN_CAN_FIFO_SIZE
#define SLCAN_CAN_FIFO_SIZE SLCAN_CAN_FIFO_DEFAULT_SIZE
#endif


//! Тип данных фифо.
typedef struct _Slcan_Can_Fifo_Data {
    slcan_can_msg_t can_msg; //!< Сообщение CAN.
    slcan_future_t* future; //!< Будущее.
} slcan_can_fifo_data_t;


//! Тип фифо.
typedef struct _Slcan_Can_Fifo {
    slcan_can_fifo_data_t buf[SLCAN_CAN_FIFO_SIZE]; //!< Данные.
    size_t wptr; //!< Индекс для записи.
    size_t rptr; //!< Индекс для чтения.
    size_t count; //!< Количество данных.
} slcan_can_fifo_t;


/**
 * Инициализирует фифо.
 * @param fifo Фифо.
 */
EXTERN void slcan_can_fifo_init(slcan_can_fifo_t* fifo);

/**
 * Сбрасывает фифо.
 * @param fifo Фифо.
 */
ALWAYS_INLINE static void slcan_can_fifo_reset(slcan_can_fifo_t* fifo)
{
    fifo->wptr = 0;
    fifo->rptr = 0;
    fifo->count = 0;
}

/**
 * Получает количество доступных для чтения данных.
 * @param fifo Фифо.
 * @return Количество доступных для чтения данных.
 */
ALWAYS_INLINE static size_t slcan_can_fifo_avail(const slcan_can_fifo_t* fifo)
{
    return fifo->count;
}

/**
 * Получает оставшееся место для записи данных.
 * @param fifo Фифо.
 * @return Размер данных, которые могут быть записаны.
 */
ALWAYS_INLINE static size_t slcan_can_fifo_remain(const slcan_can_fifo_t* fifo)
{
    return SLCAN_CAN_FIFO_SIZE - fifo->count;
}

/**
 * Получает флаг заполненности фифо.
 * @param fifo Фифо.
 * @return Флаг заполненности фифо.
 */
ALWAYS_INLINE static bool slcan_can_fifo_full(const slcan_can_fifo_t* fifo)
{
    return fifo->count == SLCAN_CAN_FIFO_SIZE;
}

/**
 * Получает флаг пустоты фифо.
 * @param fifo Фифо.
 * @return Флаг пустоты фифо.
 */
ALWAYS_INLINE static bool slcan_can_fifo_empty(const slcan_can_fifo_t* fifo)
{
    return fifo->count == 0;
}

/**
 * Помещает данные в фифо.
 * @param fifo Фифо.
 * @param msg Указатель на сообщение CAN.
 * @param future Указатель на будущее.
 * @return Количество помещённых данных, 0 - при невозможности поместить данные (фифо полное).
 */
EXTERN size_t slcan_can_fifo_put(slcan_can_fifo_t* fifo, const slcan_can_msg_t* msg, slcan_future_t* future);

/**
 * Получает данные из фифо.
 * @param fifo Фифо.
 * @param msg Указатель на сообщение CAN для получения.
 * @param future Указатель на указатель на будущее для получения.
 * @return Количество полученных данных, 0 - при невозможности получить данные (фифо пустое).
 */
EXTERN size_t slcan_can_fifo_get(slcan_can_fifo_t* fifo, slcan_can_msg_t* msg, slcan_future_t** future);

/**
 * Получает данные из фифо, не убирая их.
 * @param fifo Фифо.
 * @param msg Указатель на сообщение CAN для получения.
 * @param future Указатель на указатель на будущее для получения.
 * @return Количество полученных данных, 0 - при невозможности получить данные (фифо пустое).
 */
EXTERN size_t slcan_can_fifo_peek(const slcan_can_fifo_t* fifo, slcan_can_msg_t* msg, slcan_future_t** future);

/**
 * Оповещает фифо о чтении заданного размера данных.
 * @param fifo Фифо.
 * @param data_size Размер данных.
 */
EXTERN void slcan_can_fifo_data_readed(slcan_can_fifo_t* fifo, size_t data_size);

/**
 * Оповещает фифо о записи заданного размера данных.
 * @param fifo Фифо.
 * @param data_size Размер данных.
 */
EXTERN void slcan_can_fifo_data_written(slcan_can_fifo_t* fifo, size_t data_size);


#endif /* SLCAN_CAN_FIFO_H_ */
