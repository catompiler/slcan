#ifndef SLCAN_CONF_H_
#define SLCAN_CONF_H_


//! Версия аппаратного обеспечения.
#define SLCAN_HW_VERSION 0x01

//! Версия программного обеспечения.
#define SLCAN_SW_VERSION 0x03

//! Серийный номер.
#define SLCAN_SERIAL_NUMBER 0x1234


//! Тайм-аут запроса по-умолчанию, секунд.
#define SLCAN_MASTER_TIMEOUT_S_DEFAULT 0
//! Тайм-аут запроса по-умолчанию, наносекунд.
#define SLCAN_MASTER_TIMEOUT_NS_DEFAULT 100000000

//! Включения отправки временных меток по-умолчанию.
#define SLCAN_SLAVE_TIMESTAMP_DEFAULT 0
//! Включение автоматической отправки принятых сообщений по-умолчанию.
#define SLCAN_SLAVE_AUTO_POLL_DEFAULT 0


//! Размер буфера команды по-умолчанию.
#define SLCAN_CMD_BUF_DEFAULT_SIZE 32


//! Размер фифо ввода-вывода по-умолчанию.
#define SLCAN_IO_FIFO_DEFAULT_SIZE 256

//! Размер фифо сообщений CAN по-умолчанию.
#define SLCAN_CAN_FIFO_DEFAULT_SIZE 32


//! Флаг поллинга io мастером.
#define SLCAN_MASTER_POLL_SLCAN 1

//! Флаг поллинга io ведомым.
#define SLCAN_SLAVE_POLL_SLCAN 1


#endif /* SLCAN_CONF_H_ */
