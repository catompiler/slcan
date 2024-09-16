#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
// slcan
#include "slcan.h"
#include "slcan_serial_io_cygwin.h"
#include "slcan_get_timestamp_cygwin.h"


static int init_slcan_master(slcan_t* sc, slcan_serial_io_t* sio, const char* serial_port_name)
{
    if(slcan_init(sc, sio, slcan_get_timestamp_cygwin) != 0){
        printf("Cann't init slcan!\n");
        return -1;
    }

    if(slcan_open(sc, serial_port_name) != 0){
        printf("Cann't open serial port: %s\n", serial_port_name);
        return -1;
    }

    slcan_port_conf_t port_conf;
    slcan_get_default_port_config(&port_conf);

    if(slcan_configure(sc, &port_conf) != 0){
        printf("Error configuring serial port!\n");
        slcan_deinit(sc);
        return -1;
    }

    return 0;
}


static int init_slcan_slave(slcan_t* sc, slcan_serial_io_t* sio, const char* serial_port_name)
{
    if(slcan_init(sc, sio, slcan_get_timestamp_cygwin) != 0){
        printf("Cann't init slcan!\n");
        return -1;
    }

    if(slcan_open(sc, serial_port_name) != 0){
        printf("Cann't open serial port: %s\n", serial_port_name);
        return -1;
    }

    slcan_port_conf_t port_conf;
    slcan_get_default_port_config(&port_conf);

    if(slcan_configure(sc, &port_conf) != 0){
        printf("Error configuring serial port!\n");
        slcan_deinit(sc);
        return -1;
    }

    return 0;
}


int main(int argc, char* argv[])
{
    static slcan_t msc;
    static slcan_t ssc;

    slcan_serial_io_t sio;
    slcan_serial_io_cygwin_init(&sio);

    const char* master_serial_port_name = "/dev/ttyS20";
    const char* slave_serial_port_name = "/dev/ttyS21";

    int res;

    res = init_slcan_master(&msc, &sio, master_serial_port_name);
    if(res == -1){
        printf("Error init master slcan!\n");
        return -1;
    }

    res = init_slcan_slave(&ssc, &sio, slave_serial_port_name);
    if(res == -1){
        printf("Error init slave slcan!\n");
        slcan_deinit(&msc);
        return -1;
    }
    
    printf("Polling...\n");

    slcan_cmd_t mcmd;
    slcan_cmd_t scmd;

    memset(&mcmd, 0x0, sizeof(slcan_cmd_t));
    memset(&scmd, 0x0, sizeof(slcan_cmd_t));


    mcmd.type = SLCAN_CMD_TRANSMIT;
    mcmd.mode = SLCAN_CMD_MODE_NONE;
    mcmd.transmit.can_msg.frame_type = SLCAN_MSG_FRAME_TYPE_NORMAL;
    mcmd.transmit.can_msg.id_type = SLCAN_MSG_ID_NORMAL;
    mcmd.transmit.can_msg.id = 0xa5;
    mcmd.transmit.can_msg.data_size = 8;
    mcmd.transmit.can_msg.data[0] = 0x01;
    mcmd.transmit.can_msg.data[1] = 0x02;
    mcmd.transmit.can_msg.data[2] = 0x03;
    mcmd.transmit.can_msg.data[3] = 0x04;
    mcmd.transmit.can_msg.data[4] = 0x05;
    mcmd.transmit.can_msg.data[5] = 0x06;
    mcmd.transmit.can_msg.data[6] = 0x07;
    mcmd.transmit.can_msg.data[7] = 0x08;
    mcmd.transmit.extdata.autopoll_flag = true;
    mcmd.transmit.extdata.has_timestamp = true;
    mcmd.transmit.extdata.timestamp = 0x1234;

    slcan_put_cmd(&msc, &mcmd);


    int max_polls = 100;

    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = 1000000; // 1 ms.
    while((1) && (--max_polls != 0)){
        slcan_poll(&msc);
        slcan_poll(&ssc);
        if(slcan_get_cmd(&ssc, &scmd)){
            break;
        }
        nanosleep(&ts, NULL);
    }

    printf("Done.\n");

    slcan_deinit(&msc);
    slcan_deinit(&ssc);

    return 0;
}
