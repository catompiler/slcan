/**
 * @file slcan_utils.h
 * Общие часто встречающиеся утилитарные функции и макросы.
 */

#ifndef SLCAN_UTILS_H_
#define SLCAN_UTILS_H_

#include <stdint.h>
#include "slcan_defs.h"


/**
 * Максимальное значение из двух.
 */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/**
 * Минимальное значнеие из двух.
 */
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

/**
 * Абсолютное значение числа.
 */
#ifndef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))
#endif

/**
 * Значение в границах.
 */
#ifndef CLAMP
#define CLAMP(v, min_v, max_v) (MIN(MAX((v), (min_v)), (max_v)))
#endif


/**
 * Преобразует шестнадцатиричное число в цифру.
 * @param digit Число.
 * @return Цифра.
 */
ALWAYS_INLINE static uint8_t digit_num_to_hex(uint8_t num)
{
    if(num < 0xa) return num + '0';
    if(num < 0x10) return num - 0xa + 'A';
    return num;
}

/**
 * Преобразует шестнадцатиричную цифру в число.
 * @param digit Цифра.
 * @return Число.
 */
ALWAYS_INLINE static uint8_t digit_hex_to_num(uint8_t digit)
{
    if(digit >= 'A' && digit <= 'F') return digit - 'A' + 0xa;
    if(digit >= 'a' && digit <= 'f') return digit - 'a' + 0xa;
    if(digit >= '0' && digit <= '9') return digit - '0';
    return digit;
}

//! Добавляет метку времени TPU к TPV с занесением результата в TPR.
#define slcan_timespec_add(TPU, TPV, TPR)\
    do{\
        (TPR)->tv_sec  = (TPU)->tv_sec  + (TPV)->tv_sec;\
        (TPR)->tv_nsec = (TPU)->tv_nsec + (TPV)->tv_nsec;\
        if((TPR)->tv_nsec >= 1000000000){\
            (TPR)->tv_nsec -= 1000000000;\
            (TPR)->tv_sec  += 1;\
        }\
    }while(0)

//! Вычитает метку времени TPV из TPU с занесением результата в TPR.
#define slcan_timespec_sub(TPU, TPV, TPR)\
    do{\
        (TPR)->tv_sec  = (TPU)->tv_sec  - (TPV)->tv_sec;\
        (TPR)->tv_nsec = (TPU)->tv_nsec - (TPV)->tv_nsec;\
        if((TPR)->tv_nsec < 0){\
            (TPR)->tv_nsec += 1000000000;\
            (TPR)->tv_sec  -= 1;\
        }\
    while(0)

//! Осуществляет операцию сравнения cmp меток времени TPU и TPV.
#define slcan_timespec_cmp(TPU, TPV, cmp)\
        (((TPU)->tv_sec == (TPV)->tv_sec) ?\
                ((TPU)->tv_nsec cmp (TPV)->tv_nsec) :\
                ((TPU)->tv_sec cmp (TPV)->tv_sec))


#endif /* SLCAN_UTILS_H_ */
