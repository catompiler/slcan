#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
// slcan
#include "slcan.h"
#include "slcan_serial_io_cygwin.h"
#include "slcan_get_timestamp_cygwin.h"



int main(int argc, char* argv[])
{
    const char* serial_port_name = "/dev/ttyS21";

    slcan_t sc;
    slcan_serial_io_t sio;

    slcan_serial_io_cygwin_init(&sio);

    if(slcan_init(&sc, &sio, slcan_get_timestamp_cygwin) != 0){
        printf("Cann't init slcan!\n");
        return -1;
    }

    if(slcan_open(&sc, serial_port_name) != 0){
        printf("Cann't open serial port: %s\n", serial_port_name);
        return -1;
    }

    slcan_port_conf_t port_conf;
    slcan_get_default_port_config(&port_conf);

    if(slcan_configure(&sc, &port_conf) != 0){
        printf("Error configuring serial port!\n");
        slcan_deinit(&sc);
        return -1;
    }
    
    printf("Polling...\n");

    slcan_can_msg_t can_msg;

    int max_polls = 1000;

    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = 1000000; // 1 ms.
    while((slcan_poll(&sc) == 0) && (--max_polls != 0)){
        while(slcan_get_can_message(&sc, &can_msg)){
            slcan_put_can_message(&sc, &can_msg);
        }
        nanosleep(&ts, NULL);
    }

    printf("Done.\n");

    slcan_deinit(&sc);

    return 0;
}
