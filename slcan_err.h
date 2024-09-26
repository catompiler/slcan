#ifndef SLCAN_ERR_H_
#define SLCAN_ERR_H_


#include <stdint.h>


//! Перечисление ошибок SLCAN.
enum _Slcan_Errors {
    E_SLCAN_NO_ERROR      =  0,
    E_SLCAN_NULL_POINTER  =  1,
    E_SLCAN_INVALID_VALUE =  2,
    E_SLCAN_INVALID_SIZE  =  3,
    E_SLCAN_INVALID_DATA  =  4,
    E_SLCAN_OUT_OF_RANGE  =  5,
    E_SLCAN_UNDERFLOW     =  6,
    E_SLCAN_OVERFLOW      =  7,
    E_SLCAN_OVERRUN       =  8,
    E_SLCAN_UNDERRUN      =  9,
    E_SLCAN_IO_ERROR      = 10,
    E_SLCAN_UNEXPECTED    = 11,
    E_SLCAN_EXEC_FAIL     = 12,
    E_SLCAN_TIMEOUT       = 13,
    E_SLCAN_CANCELED      = 14,
    //E_SLCAN_,
};

//! Тип ошибки SLCAN.
typedef uint32_t slcan_err_t;


#endif /* SLCAN_ERR_H_ */
