#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>
// slcan
#include "slcan.h"
#include "slcan_master.h"
#include "slcan_slave.h"


#ifdef __linux
// socat -d -d pty,rawer,echo=0,link=/tmp/ttyV0,perm=0777 pty,rawer,echo=0,link=/tmp/ttyV1,perm=0777
#define MASTER_TTY "/tmp/ttyV0"
#define SLAVE_TTY "/tmp/ttyV1"
#else
// cygwin + com0com
#define MASTER_TTY "/dev/ttyS20"
#define SLAVE_TTY "/dev/ttyS21"
#endif


static int init_slcan_master(slcan_master_t* scm, slcan_t* sc, const char* serial_port_name)
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

    if(slcan_configure(sc, &port_conf) != 0){
        printf("Error configuring serial port!\n");
        slcan_deinit(sc);
        return -1;
    }

    if(slcan_master_init(scm, sc) != 0){
        printf("Error init slcan master!\n");
        slcan_deinit(sc);
        return -1;
    }

    return 0;
}


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

    return 0;
}


static slcan_err_t on_setup_can_std(slcan_bit_rate_t bit_rate)
{
    printf("setup can std: 0x%02x\n", (int)bit_rate);
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_setup_can_btr(uint8_t btr0, uint8_t btr1)
{
    printf("setup can btr: 0x%x 0x%02x\n", (int)btr0, (int)btr1);
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_open(void)
{
    printf("open can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_listen(void)
{
    printf("listen can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_close(void)
{
    printf("close can\n");
    return E_SLCAN_NO_ERROR;
}

static slcan_err_t on_setup_uart(slcan_port_baud_t baud)
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


int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;
    
    srand(time(NULL));

    static slcan_t master_slcan;
    static slcan_t slave_slcan;

    slcan_slave_callbacks_t scb;

    static slcan_master_t master;
    static slcan_slave_t slave;

    scb.on_setup_can_std = on_setup_can_std;
    scb.on_setup_can_btr = on_setup_can_btr;
    scb.on_open = on_open;
    scb.on_listen = on_listen;
    scb.on_close = on_close;
    scb.on_setup_uart = on_setup_uart;

    const char* master_serial_port_name = MASTER_TTY;
    const char* slave_serial_port_name = SLAVE_TTY;

    int res;

    res = init_slcan_master(&master, &master_slcan, master_serial_port_name);
    if(res == -1){
        printf("Error init master slcan!\n");
        return -1;
    }

    res = init_slcan_slave(&slave, &slave_slcan, &scb, slave_serial_port_name);
    if(res == -1){
        printf("Error init slave slcan!\n");
        slcan_deinit(&master_slcan);
        return -1;
    }
    
    printf("Polling...\n");

    slcan_future_t future_slave;
    slcan_future_t future_master;

    slcan_future_init(&future_slave);
    slcan_future_init(&future_master);


    const size_t slave_can_msgs_count = 5;
    slcan_can_msg_t slave_can_msg[slave_can_msgs_count];

    const size_t master_can_msgs_count = 5;
    slcan_can_msg_t master_can_msg[master_can_msgs_count];


    int i;
    for(i = 0; i < slave_can_msgs_count - 1; i ++){
        gen_can_msg(&slave_can_msg[i]);
        slcan_slave_send_can_msg(&slave, &slave_can_msg[i], NULL);
    }
    gen_can_msg(&slave_can_msg[slave_can_msgs_count - 1]);
    slcan_slave_send_can_msg(&slave, &slave_can_msg[slave_can_msgs_count - 1], &future_slave);
    //slcan_slave_send_can_msg(&slave, &can_msg[can_msgs_count - 1], NULL);

    uint8_t hw_ver = 0, sw_ver = 0;
    uint16_t sn = 0;
    slcan_slave_status_t status;


    slcan_master_cmd_read_version(&master, &hw_ver, &sw_ver, NULL);
    slcan_master_cmd_read_sn(&master, &sn, NULL);
    slcan_master_cmd_setup_uart(&master, SLCAN_PORT_BAUD_115200, NULL);
    slcan_master_cmd_setup_can_std(&master, SLCAN_BIT_RATE_250Kbit, NULL);
    slcan_master_cmd_setup_can_btr(&master, 0x12, 0x34, NULL);
    slcan_master_cmd_set_auto_poll(&master, false, NULL);
    slcan_master_cmd_set_timestamp(&master, true, NULL);
    slcan_master_cmd_open(&master, NULL);
    slcan_master_cmd_listen(&master, NULL);
    slcan_master_cmd_poll(&master, NULL);
    slcan_master_cmd_poll_all(&master, NULL);

    for(i = 0; i < master_can_msgs_count - 1; i ++){
        gen_can_msg(&master_can_msg[i]);
        slcan_master_send_can_msg(&master, &master_can_msg[i], NULL);
    }
    gen_can_msg(&master_can_msg[master_can_msgs_count - 1]);
    //slcan_master_send_can_msg(&master, &master_can_msg[master_can_msgs_count - 1], &future_master);
    slcan_master_send_can_msg(&master, &master_can_msg[master_can_msgs_count - 1], NULL);

    slcan_master_cmd_read_status(&master, &status, NULL);
    slcan_master_cmd_close(&master, &future_master);

    int max_polls = 1000;

    struct timespec ts;
    ts.tv_sec = 0; ts.tv_nsec = 1000000; // 1 ms.
    while(--max_polls != 0){
        slcan_master_poll(&master);
        slcan_slave_poll(&slave);

        if(slcan_future_done(&future_master) && slcan_future_done(&future_slave)) break;

        nanosleep(&ts, NULL);
    }

    printf("future master done: %u result: %d\n", (unsigned int)slcan_future_done(&future_master), SLCAN_FUTURE_RESULT_INT(slcan_future_result(&future_master)));
    printf("future slave done: %u result: %d\n", (unsigned int)slcan_future_done(&future_slave), SLCAN_FUTURE_RESULT_INT(slcan_future_result(&future_slave)));

    printf("hw version: 0x%02x\n", (unsigned int)hw_ver);
    printf("sw version: 0x%02x\n", (unsigned int)sw_ver);
    printf("s/n: 0x%04x\n", (unsigned int)sn);

    printf("status: 0x%02x\n", (unsigned int)status);

    printf("Done.\n");

    slcan_deinit(&master_slcan);
    slcan_deinit(&slave_slcan);

    return 0;
}
