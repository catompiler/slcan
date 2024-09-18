#ifndef SLCAN_FUTURE_H_
#define SLCAN_FUTURE_H_


#include <stdbool.h>
#include <stddef.h>
#include "defs/defs.h"
#include "slcan_err.h"


#define SLCAN_FUTURE_RESULT(RES) ((void*)(RES))

#define SLCAN_FUTURE_RESULT_CAST(T, RES) ((T)(RES))
#define SLCAN_FUTURE_RESULT_CAST_LONG(RES) SLCAN_FUTURE_RESULT_CAST(long, (RES))
#define SLCAN_FUTURE_RESULT_CAST_ULONG(RES) SLCAN_FUTURE_RESULT_CAST(unsigned long, (RES))
#define SLCAN_FUTURE_RESULT_INT(RES) ((int)SLCAN_FUTURE_RESULT_CAST_LONG(RES))
#define SLCAN_FUTURE_RESULT_UINT(RES) ((unsigned int)SLCAN_FUTURE_RESULT_CAST_LONG(RES))
#define SLCAN_FUTURE_RESULT_ERR(RES) ((slcan_err_t)SLCAN_FUTURE_RESULT_CAST_LONG(RES))


/**
 * Структура будущего.
 */
typedef struct _Slcan_Future {
    //! Результат.
    void * volatile result;
    //! Флаг окончания.
    volatile bool done;
    //! Флаг выполнения.
    volatile bool running;
} slcan_future_t;

/**
 * Инициализирует будущее.
 * Незавершённое, невыполняющееся, с нулевым результатом.
 * @param future Будущее.
 */
ALWAYS_INLINE static void slcan_future_init(slcan_future_t* future)
{
    future->result = NULL;
    future->done = false;
    future->running = false;
}

/**
 * Получает результат будущего.
 * @param future Будущее.
 * @return Результат.
 */
ALWAYS_INLINE static void* slcan_future_result(const slcan_future_t* future)
{
    return future->result;
}

/**
 * Устанавливает результат.
 * @param future Будущее.
 * @param result Результат.
 */
ALWAYS_INLINE static void slcan_future_set_result(slcan_future_t* future, void* result)
{
    future->result = result;
}

/**
 * Получает флаг завершения.
 * @param future Будущее.
 * @return Флаг завершения.
 */
ALWAYS_INLINE static bool slcan_future_done(const slcan_future_t* future)
{
    return future->done;
}

/**
 * Устанавливает флаг завершения.
 * @param future Будущее.
 * @param done Флаг завершения.
 */
ALWAYS_INLINE static void slcan_future_set_done(slcan_future_t* future, bool done)
{
    future->done = done;
}

/**
 * Получает флаг выполнения.
 * @param future Будущее.
 * @return Флаг выполнения.
 */
ALWAYS_INLINE static bool slcan_future_running(const slcan_future_t* future)
{
    return future->running;
}

/**
 * Устанавливает флаг выполнения.
 * @param future Будущее.
 * @param running Флаг выполнения.
 */
ALWAYS_INLINE static void slcan_future_set_running(slcan_future_t* future, bool running)
{
    future->running = running;
}

/**
 * Запускает выполнение будущего (done = false; running = true;).
 * @param future Будущее.
 */
ALWAYS_INLINE static void slcan_future_start(slcan_future_t* future)
{
    future->done = false;
    future->running = true;
}

/**
 * Завершает выполнение будущего (done = true; running = false;).
 * @param future Будущее.
 * @param result Результат.
 */
ALWAYS_INLINE static void slcan_future_finish(slcan_future_t* future, void* result)
{
    future->done = true;
    future->running = false;
    future->result = result;
}

/**
 * Ждёт окончания выполнения будущего.
 * @param future Будущее.
 */
ALWAYS_INLINE static void slcan_future_wait(const slcan_future_t* future)
{
    for(;;){
        if(!slcan_future_running(future)) break;
    }
}





#endif /* SLCAN_FUTURE_H_ */
