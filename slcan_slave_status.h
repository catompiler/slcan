#ifndef SLCAN_SLAVE_STATUS_H_
#define SLCAN_SLAVE_STATUS_H_


//! Статус ведомого устройства SLCAN.
typedef enum _Slcan_Slave_Status {
    SLCAN_SLAVE_STATUS_NONE             = 0,
    SLCAN_SLAVE_STATUS_RX_FIFO_FULL     = 0x1,
    SLCAN_SLAVE_STATUS_TX_FIFO_FULL     = 0x2,
    SLCAN_SLAVE_STATUS_ERROR_WARNING    = 0x4,
    SLCAN_SLAVE_STATUS_OVERRUN          = 0x8,
    SLCAN_SLAVE_STATUS_NOT_USED         = 0x10,
    SLCAN_SLAVE_STATUS_ERROR_PASSIVE    = 0x20,
    SLCAN_SLAVE_STATUS_ARBITRATION_LOST = 0x40,
    SLCAN_SLAVE_STATUS_BUS_ERROR        = 0x80,
} slcan_slave_status_t;


#endif /* SLCAN_SLAVE_STATUS_H_ */
