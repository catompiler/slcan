#ifndef SLCAN_PORT_H_
#define SLCAN_PORT_H_


#include <time.h>
#include "defs/defs.h"


EXTERN WEAK int slcan_clock_gettime (clockid_t clock_id, struct timespec *tp);


#endif /* SLCAN_PORT_H_ */
