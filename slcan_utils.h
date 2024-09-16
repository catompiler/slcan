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

#endif /* SLCAN_UTILS_H_ */
