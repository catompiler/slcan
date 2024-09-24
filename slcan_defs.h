/**
 * @file slcan_defs.h
 * Общие часто встречающиеся макросы.
 */

#ifndef SLCAN_DEFS_H
#define	SLCAN_DEFS_H

#ifndef EXTERN
    #ifdef __cplusplus
        #define EXTERN extern "C"
    #else
        #define EXTERN extern
    #endif
#endif

#ifndef ALWAYS_INLINE
    #define ALWAYS_INLINE inline  __attribute__((always_inline))
#endif

#endif	/* SLCAN_DEFS_H */

