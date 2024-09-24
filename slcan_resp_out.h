#ifndef SLCAN_RESP_OUT_H_
#define SLCAN_RESP_OUT_H_


#include <stdint.h>
#include <time.h>
#include "slcan_future.h"
#include "slcan_cmd.h"
#include "slcan_slave_status.h"


//! Структура данных отправленного запроса настройки CAN на стандартный битрейт.
typedef struct _Slcan_Resp_Out_Setup_Can_Std {
} slcan_resp_out_setup_can_std_t;

//! Структура данных отправленного запроса настройки CAN на нестандартный битрейт.
typedef struct _Slcan_Resp_Out_Setup_Can_Btr {
} slcan_resp_out_setup_can_btr_t;

//! Структура данных отправленного запроса открытия CAN.
typedef struct _Slcan_Resp_Out_Open {
} slcan_resp_out_open_t;

//! Структура данных отправленного запроса открытия CAN только на прослушку.
typedef struct _Slcan_Resp_Out_Listen {
} slcan_resp_out_listen_t;

//! Структура данных отправленного запроса закрытия CAN.
typedef struct _Slcan_Resp_Out_Close {
} slcan_resp_out_close_t;

//! Структура данных отправленного запроса передачи сообщения CAN.
typedef struct _Slcan_Resp_Out_Transmit {
} slcan_resp_out_transmit_t;

//! Структура данных отправленного запроса получения принятого сообщения CAN.
typedef struct _Slcan_Resp_Out_Poll {
} slcan_resp_out_poll_t;

//! Структура данных отправленного запроса получения всех принятых сообщений CAN.
typedef struct _Slcan_Resp_Out_Poll_All {
} slcan_resp_out_poll_all_t;

//! Структура данных отправленного запроса получения статуса.
typedef struct _Slcan_Resp_Out_Status {
    slcan_slave_status_t* status;
} slcan_resp_out_status_t;

//! Структура данных отправленного запроса установки режима автоматического получения принятых сообщений CAN.
typedef struct _Slcan_Resp_Out_Set_Auto_Poll {
} slcan_resp_out_set_auto_poll_t;

//! Структура данных отправленного запроса настройки UART.
typedef struct _Slcan_Resp_Out_Setup_Uart {
} slcan_resp_out_setup_uart_t;

//! Структура данных отправленного запроса получения версии.
typedef struct _Slcan_Resp_Out_Version {
    uint8_t* hw_version;
    uint8_t* sw_version;
} slcan_resp_out_version_t;

//! Структура данных отправленного запроса получения серийного номера.
typedef struct _Slcan_Resp_Out_Sn {
    uint16_t* sn;
} slcan_resp_out_sn_t;

//! Структура данных отправленного запроса установки получения отметок времени.
typedef struct _Slcan_Resp_Out_Set_Timestamp {
} slcan_resp_out_set_timestamp_t;


//! Структура общего отправленного запроса.
typedef struct _Slcan_Resp_Out {
    slcan_cmd_type_t req_type; //!< Тип запроса (@see slcan_cmd_type_t).
    slcan_future_t* future; //!< Указатель на будущее для сигнализации о завершении запроса.
    struct timespec tp_req; //!< Время истечения тайм-аута.
    //! Объединение всех видов отправленных запросов.
    union {
        slcan_resp_out_setup_can_std_t setup_can_std;
        slcan_resp_out_setup_can_btr_t setup_can_btr;
        slcan_resp_out_open_t open;
        slcan_resp_out_listen_t listen;
        slcan_resp_out_close_t close;
        slcan_resp_out_transmit_t transmit;
        slcan_resp_out_poll_t poll;
        slcan_resp_out_poll_all_t poll_all;
        slcan_resp_out_status_t status;
        slcan_resp_out_set_auto_poll_t set_auto_poll;
        slcan_resp_out_setup_uart_t setup_uart;
        slcan_resp_out_version_t version;
        slcan_resp_out_sn_t sn;
        slcan_resp_out_set_timestamp_t set_timestamp;
    };
} slcan_resp_out_t;


#endif /* SLCAN_RESP_OUT_H_ */
