#ifndef SLCAN_UTILS_H_
#define SLCAN_UTILS_H_

#include <stdint.h>
#include "defs/defs.h"



ALWAYS_INLINE static uint8_t digit_num_to_hex(uint8_t digit)
{
    if(digit < 0xa) return digit + '0';
    if(digit < 0x10) return digit - 0xa + 'A';
    return digit;
}

ALWAYS_INLINE static uint8_t digit_hex_to_num(uint8_t digit)
{
    if(digit >= 'A' && digit <= 'F') return digit - 'A' + 0xa;
    if(digit >= 'a' && digit <= 'f') return digit - 'a' + 0xa;
    if(digit >= '0' && digit <= '9') return digit - '0';
    return digit;
}


#define slcan_timespec_add(TPU, TPV, TPR)\
    do{\
        (TPR)->tv_sec  = (TPU)->tv_sec  + (TPV)->tv_sec;\
        (TPR)->tv_nsec = (TPU)->tv_nsec + (TPV)->tv_nsec;\
        if((TPR)->tv_nsec >= 1000000000){\
            (TPR)->tv_nsec -= 1000000000;\
            (TPR)->tv_sec  += 1;\
        }\
    }while(0)

#define slcan_timespec_sub(TPU, TPV, TPR)\
    do{\
        (TPR)->tv_sec  = (TPU)->tv_sec  - (TPV)->tv_sec;\
        (TPR)->tv_nsec = (TPU)->tv_nsec - (TPV)->tv_nsec;\
        if((TPR)->tv_nsec < 0){\
            (TPR)->tv_nsec += 1000000000;\
            (TPR)->tv_sec  -= 1;\
        }\
    while(0)

#define slcan_timespec_cmp(TPU, TPV, cmp)\
        (((TPU)->tv_sec == (TPV)->tv_sec) ?\
                ((TPU)->tv_nsec cmp (TPV)->tv_nsec) :\
                ((TPU)->tv_sec cmp (TPV)->tv_sec))


#endif /* SLCAN_UTILS_H_ */
