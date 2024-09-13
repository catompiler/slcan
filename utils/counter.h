#ifndef UTILS_COUNTER_H
#define UTILS_COUNTER_H


#include <stdint.h>
#include "defs/defs.h"


/**
 * Вычисляет разницу двух счётчиков
 * с учётом возможного переполнения.
 * @param cur Текущее значение счётчика.
 * @param ref Опорное (прошлое) значение счётчика.
 * @return Разница.
 */
ALWAYS_INLINE static uint32_t counter_calc_diff(uint32_t cur, uint32_t ref)
{
    // Текущее значение больше опорного - счётчик не переполнился.
    if(cur >= ref) return cur - ref;
    // Иначе cur < ref - счётчик переполнился.
    return UINT32_MAX - ref + cur + 1;
}


#endif /* UTILS_COUNTER_H */
