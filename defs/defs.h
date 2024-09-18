/**
 * @file defs.h
 * Общие часто встречающиеся макросы.
 */

#ifndef DEFS_H
#define	DEFS_H

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

#define ALWAYS_INLINE inline  __attribute__((always_inline))

#define WEAK __attribute__((weak))

#define PACKED __attribute__((packed))

#define ALIGNED(N) __attribute__((aligned(N)))
#define ALIGNED1 ALIGNED(1)
#define NOALIGNED ALIGNED1
#define ALIGNED4 ALIGNED(4)

#define STRING(s) #s
#define MAKE_STRING(s) STRING(s)
#define CONCAT_SIMPLE(a, ...) a ## __VA_ARGS__
#define CONCAT_SIMPLE3(a, b, ...) a ## b ## __VA_ARGS__
#define CONCAT(a, ...) CONCAT_SIMPLE(a, __VA_ARGS__)

#endif	/* DEFS_H */

