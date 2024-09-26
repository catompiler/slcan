#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
// slcan
#include "slcan.h"
#include "slcan_slave.h"


#ifdef __linux
// socat -d -d pty,rawer,echo=0,link=/tmp/ttyV0,perm=0777 pty,rawer,echo=0,link=/tmp/ttyV1,perm=0777
#define SLAVE_TTY "/tmp/ttyV1"
#else
// cygwin + com0com
#define SLAVE_TTY "/dev/ttyS21"
#endif


static int init_slcan_slave(slcan_slave_t* scs, slcan_t* sc, slcan_slave_callbacks_t* scb, const char* serial_port_name)
{
    if(slcan_init(sc) != 0){
        printf("Cann't init slcan!\n");
        return -1;
    }

    if(slcan_open(sc, serial_port_name) != 0){
        printf("Cann't open serial port: %s\n", serial_port_name);
        return -1;
    }

    slcan_port_conf_t port_conf;
    slcan_get_default_port_config(&port_conf);

    port_conf.baud = SLCAN_PORT_BAUD_115200;

    if(slcan_configure(sc, &port_conf) != 0){
        printf("Error configuring serial port!\n");
        slcan_deinit(sc);
        return -1;
    }

    if(slcan_slave_init(scs, sc, scb) != 0){
        printf("Error init slcan slave!\n");
        slcan_deinit(sc);
        return -1;
    }

    slcan_slave_set_flags(scs,
            slcan_slave_flags(scs) |
            SLCAN_SLAVE_FLAG_OPENED |
            SLCAN_SLAVE_FLAG_LISTEN_ONLY |
            SLCAN_SLAVE_FLAG_AUTO_POLL);

    return 0;
}


static slcan_err_t on_setup_can_std(slcan_bit_rate_t bit_rate, void* user_data)
{
    printf("setup can std: 0x%02x\n", (int)bit_rate);
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_setup_can_btr(uint8_t btr0, uint8_t btr1, void* user_data)
{
    printf("setup can btr: 0x%x 0x%02x\n", (int)btr0, (int)btr1);
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_open(void* user_data)
{
    printf("open can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_listen(void* user_data)
{
    printf("listen can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_close(void* user_data)
{
    printf("close can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_setup_uart(slcan_port_baud_t baud, void* user_data)
{
    printf("setup uart: 0x%02x\n", (int)baud);
    return E_SLCAN_NO_ERROR;
}


static void gen_can_msg(slcan_can_msg_t* msg)
{
    if(msg == NULL) return;

    bool id_ext = rand() & 0x1;
    bool rtr = rand() & 0x1;
    size_t dlc = rand() % 9;
    uint8_t data[8];
    uint32_t id;

    int i;

    if(id_ext){
        id = rand() % 0x1fffffff;
    }else{
        id = rand() % 0x7ff;
    }

    if(!rtr){
        for(i = 0; i < dlc; i ++){
            data[i] = rand() & 0xff;
        }
    }

    msg->frame_type = rtr ? SLCAN_CAN_FRAME_RTR : SLCAN_CAN_FRAME_NORMAL;
    msg->id_type = id_ext ? SLCAN_CAN_ID_EXTENDED : SLCAN_CAN_ID_NORMAL;
    msg->id = id;
    msg->dlc = dlc;
    if(!rtr){
        for(i = 0; i < dlc; i ++){
            msg->data[i] = data[i];
        }
    }
}


int main_slave_recv_rand(int argc, char* argv[])
{
    (void) argc;
    (void) argv;
    
    srand(time(NULL));

    static slcan_t slave_slcan;

    slcan_slave_callbacks_t scb;

    static slcan_slave_t slave;

    memset(&scb, 0x0, sizeof(slcan_slave_callbacks_t));
    scb.on_setup_can_std = on_setup_can_std;
    scb.on_setup_can_btr = on_setup_can_btr;
    scb.on_open = on_open;
    scb.on_listen = on_listen;
    scb.on_close = on_close;
    scb.on_setup_uart = on_setup_uart;

    const char* slave_serial_port_name = SLAVE_TTY;

    int res;

    res = init_slcan_slave(&slave, &slave_slcan, &scb, slave_serial_port_name);
    if(res == -1){
        printf("Error init slave slcan!\n");
        return -1;
    }

    printf("Polling...\n");


    slcan_can_msg_t can_msg;

    int max_polls = 1000;

    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = 1000000; // 1 ms.
    while(--max_polls != 0){
        slcan_slave_poll(&slave);

        // send random msg.
        gen_can_msg(&can_msg);
        slcan_slave_send_can_msg(&slave, &can_msg, NULL);

        nanosleep(&ts, NULL);
    }

    struct timespec tp_timeout = {1, 0};
    slcan_err_t err = slcan_slave_flush(&slave, &tp_timeout);
    if(err != E_SLCAN_NO_ERROR) printf("slcan flush err: %d\n", (int)err);

    printf("Done.\n");

    slcan_slave_deinit(&slave);
    slcan_close(&slave_slcan);
    slcan_deinit(&slave_slcan);

    return 0;
}

#if defined(EXAMPLE_SLAVE_RECV_RAND) && EXAMPLE_SLAVE_RECV_RAND == 1
int main(int argc, char* argv[])
{
    return main_slave_recv_rand(argc, argv);
}
#endif
