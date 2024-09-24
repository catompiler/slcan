#ifndef SLCAN_CMD_BUF_H_
#define SLCAN_CMD_BUF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "slcan_defs.h"
#include "slcan_utils.h"
#include "slcan_conf.h"


//! Размер буфера команды.
#ifndef SLCAN_CMD_BUF_SIZE
#define SLCAN_CMD_BUF_SIZE SLCAN_CMD_BUF_DEFAULT_SIZE
#endif


//! Буфер команды.
typedef struct _Slcan_Cmd_Buf {
    uint8_t buf[SLCAN_CMD_BUF_SIZE]; //!< Данные.
    size_t size; //!< Текущий размер данных в буфере.
} slcan_cmd_buf_t;


/**
 * Инициализирует буфер.
 * @param buf Буфер.
 */
EXTERN void slcan_cmd_buf_init(slcan_cmd_buf_t* buf);

/**
 * Сбрасывает буфер.
 * @param buf Буфер.
 */
ALWAYS_INLINE static void slcan_cmd_buf_reset(slcan_cmd_buf_t* buf)
{
    buf->size = 0;
}

/**
 * Получает указатель на данные буфера.
 * @param buf Буфер.
 * @return Указатель на данные буфера.
 */
ALWAYS_INLINE static uint8_t* slcan_cmd_buf_data(slcan_cmd_buf_t* buf)
{
    return buf->buf;
}

/**
 * Получает константный указатель на данные буфера.
 * @param buf Буфер.
 * @return Константный указатель на данные буфера.
 */
ALWAYS_INLINE static const uint8_t* slcan_cmd_buf_data_const(const slcan_cmd_buf_t* buf)
{
    return buf->buf;
}

/**
 * Получает указатель на конец данных в буфере.
 * @param buf Буфер.
 * @return Указатель на конец данных в буфере.
 */
ALWAYS_INLINE static uint8_t* slcan_cmd_buf_data_end(slcan_cmd_buf_t* buf)
{
    return &buf->buf[buf->size];
}

/**
 * Получает константный указатель на конец данных в буфере.
 * @param buf Буфер.
 * @return Константный указатель на конец данных в буфере.
 */
ALWAYS_INLINE static const uint8_t* slcan_cmd_buf_data_end_const(const slcan_cmd_buf_t* buf)
{
    return &buf->buf[buf->size];
}

/**
 * Получает размер данных в буфере.
 * @param buf Буфер.
 * @return Размер данных в буфере.
 */
ALWAYS_INLINE static size_t slcan_cmd_buf_size(const slcan_cmd_buf_t* buf)
{
    return buf->size;
}

/**
 * Устанавливает размер данных в буфере.
 * @param buf Буфер.
 * @param size Размер данных.
 */
ALWAYS_INLINE static void slcan_cmd_buf_set_size(slcan_cmd_buf_t* buf, size_t size)
{
    buf->size = MIN(size, SLCAN_CMD_BUF_SIZE);
}

/**
 * Помещает в буфер байт данных.
 * @param buf Буфер.
 * @param data Байт данных.
 * @return Число помещённых в буфер байт данных, 0 - при невозможности поместить данные (буфер заполнен).
 */
EXTERN size_t slcan_cmd_buf_put(slcan_cmd_buf_t* buf, uint8_t data);

#endif /* SLCAN_CMD_BUF_H_ */
