#include "slcan_get_timestamp_cygwin.h"


time_t slcan_get_timestamp_cygwin(void)
{
    struct timespec ts;

    if(clock_gettime(CLOCK_MONOTONIC, &ts) != 0){
        return 0;
    }

    time_t t_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    return t_ms;
}
