#include "slcan_port.h"


int slcan_clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    return clock_gettime(clock_id, tp);
}

