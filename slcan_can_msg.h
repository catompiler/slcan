#ifndef SLCAN_CAN_MSG_H_
#define SLCAN_CAN_MSG_H_

#include <stdint.h>
#include <stdbool.h>
#include "defs/defs.h"
#include "slcan_err.h"
#include "slcan_cmd_buf.h"


//! Перечисление типов фреймов CAN.
typedef enum _Slcan_Can_Frame_Type {
    SLCAN_CAN_FRAME_NORMAL = 0,
    SLCAN_CAN_FRAME_RTR = 1,
} slcan_can_frame_type_t;

//! Перечисление типов идентификаторов CAN.
typedef enum _Slcan_Can_Id_Type {
    SLCAN_CAN_ID_NORMAL = 0, //!< 11 bit.
    SLCAN_CAN_ID_EXTENDED = 1, //!< 29 bit.
} slcan_can_id_type_t;

//! Максимальное значение нормального идентификатора.
#define SLCAN_CAN_ID_NORMAL_MAX 0x7ff

//! Максимальное значение расширенного индикатора.
#define SLCAN_CAN_ID_EXTENDED_MAX 0x1fffffff

//! Максимальный размер данных CAN.
#define SLCAN_CAN_DATA_SIZE_MAX 8

//! Структура сообщения CAN.
typedef struct _Slcan_Can_Msg {
    uint32_t id; //!< Идентификатор.
    uint8_t data[SLCAN_CAN_DATA_SIZE_MAX]; //!< Данные.
    uint8_t dlc; //!< Размер данных.
    uint8_t frame_type; //!< Тип фрейма.
    uint8_t id_type; //!< Тип идентификатора.
} slcan_can_msg_t;

//! Структура с дополнительными данными о сообщении CAN.
typedef struct _Slcan_Can_Msg_Extdata {
    uint16_t timestamp; //!< Отметка времени.
    bool has_timestamp; //!< Флаг наличия переданной отметки.
    bool autopoll_flag; //!< Флаг автоматической отправки полученных сообщений.
} slcan_can_msg_extdata_t;

/**
 * Проверяет сообщение CAN на валидность.
 * @param msg Сообщение CAN.
 * @return Флаг валидности.
 */
EXTERN bool slcan_can_msg_is_valid(const slcan_can_msg_t* msg);

/**
 * Десериализация сообщения CAN из буфера.
 * @param can_msg Сообщение CAN.
 * @param ed Расширенные данные о сообщении.
 * @param cmd_buf Буфер.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_can_msg_from_buf(slcan_can_msg_t* can_msg, slcan_can_msg_extdata_t* ed, const slcan_cmd_buf_t* cmd_buf);

/**
 * Сериализация сообщения CAN в буфер.
 * @param can_msg Сообщение CAN.
 * @param ed Расширенные данные о сообщении.
 * @param cmd_buf Буфер.
 * @return Код ошибки.
 */
EXTERN slcan_err_t slcan_can_msg_to_buf(const slcan_can_msg_t* can_msg, const slcan_can_msg_extdata_t* ed, slcan_cmd_buf_t* cmd_buf);


#endif /* SLCAN_CAN_MSG_H_ */
