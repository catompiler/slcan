#include "slcan_port.h"
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/ioctl.h>


int slcan_clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    return clock_gettime(clock_id, tp);
}


// Преобразует тип последовательного порта в slcan_serial_handle_t.
#define SERIAL_TO_HANDLE(S) ((slcan_serial_handle_t)(long)(S))
// Преобразует slcan_serial_handle_t в тип последовательного порта.
#define HANDLE_TO_SERIAL(H) ((int)(long)(H))


int slcan_serial_open(const char* serial_port_name, slcan_serial_handle_t* serial_port)
{
    if(serial_port == NULL) return SLCAN_IO_FAIL;

    int s = open(serial_port_name, O_RDWR | O_NOCTTY);
    if(s < 0) return SLCAN_IO_FAIL;

    *serial_port = SERIAL_TO_HANDLE(s);

    return SLCAN_IO_SUCCESS;
}

int slcan_serial_configure(slcan_serial_handle_t serial_port, const slcan_port_conf_t* conf)
{
    static const int slcan_port_bauds_impl[] = {
        B230400,
        B115200,
        B57600,
        B38400,
        B19200,
        B9600,
        B2400,
    };

    struct termios tty;

    int res;

    res = tcgetattr(HANDLE_TO_SERIAL(serial_port), &tty);
    if(res == SLCAN_IO_FAIL) return res;


    switch (conf->parity){
    default:
    case SLCAN_PORT_PARITY_NONE:
        // clear parity flags.
        tty.c_cflag &= ~(PARENB | PARODD);
        // input parity flags.
        tty.c_iflag |= IGNPAR;
        tty.c_iflag &= ~INPCK;
        break;
    case SLCAN_PORT_PARITY_EVEN:
        // clear odd flag.
        tty.c_cflag &= ~PARODD;
        // set parity flag.
        tty.c_cflag |= PARENB;
        // input parity flags.
        tty.c_iflag &= ~IGNPAR;
        tty.c_iflag |= INPCK;
        break;
    case SLCAN_PORT_PARITY_ODD:
        // set parity and odd flags.
        tty.c_cflag |= (PARENB | PARODD);
        // input parity flags.
        tty.c_iflag &= ~IGNPAR;
        tty.c_iflag |= INPCK;
        break;
    }

    switch (conf->stop_bits){
    default:
    case SLCAN_PORT_STOP_BITS_1:
        // clear stop 2 bits flag.
        tty.c_cflag &= ~CSTOPB;
        break;
    case SLCAN_PORT_STOP_BITS_2:
        // set stop 2 bits flag.
        tty.c_cflag |= CSTOPB;
        break;
    }

    // Control flags.
    // word size.
    // clear size flags.
    tty.c_cflag &= ~(CSIZE);
    // set 8 bit size flag.
    tty.c_cflag |= CS8;
    // hw flow control.
    // disable.
    tty.c_cflag &= ~CRTSCTS;
    // local mode.
    tty.c_cflag |= CLOCAL;
    // enable receiver.
    tty.c_cflag |= CREAD;


    // local mode flags.

    // canonical mode.
    // clear canonical mode flag (non-canonical mode).
    tty.c_lflag &= ~ICANON;

    // disable echo.
    tty.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

    // disable signal characters.
    tty.c_lflag &= ~ISIG;

    // disable impl-defined input processing.
    tty.c_lflag &= ~IEXTEN;


    // Input mode flags.
    // not mark bytes with parity / framing errors.
    tty.c_iflag &= ~PARMRK;
    // BREAK.
    tty.c_iflag &= ~(IGNBRK | BRKINT);

    // disable software flow control.
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // disable any special processing.
    tty.c_iflag &= ~(ISTRIP | INLCR | ICRNL | IGNCR | IUCLC | IMAXBEL | IUTF8);


    // Output mode flags.

    // disable any special processing.
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~(OLCUC | ONLCR | OCRNL  | ONOCR | ONLRET | OFILL | OFDEL |
                     NLDLY | CRDLY | TABDLY | BSDLY | VTDLY  | FFDLY);

    // Minimal characters to be read.
    tty.c_cc[VMIN] = 0;
    // Timeout to blocking read.
    tty.c_cc[VTIME] = 0;


    // Speed.
    cfsetispeed(&tty, slcan_port_bauds_impl[conf->baud]);
    cfsetospeed(&tty, slcan_port_bauds_impl[conf->baud]);


    // set conf to port.
    res = tcsetattr(HANDLE_TO_SERIAL(serial_port), TCSANOW, &tty);
    if(res == SLCAN_IO_FAIL) return res;


    // set non blocking io.
    int flags = fcntl(HANDLE_TO_SERIAL(serial_port), F_GETFL, 0);
    if(flags == SLCAN_IO_FAIL) return flags;

    res = fcntl(HANDLE_TO_SERIAL(serial_port), F_SETFL, flags | O_NONBLOCK);
    if(res == SLCAN_IO_FAIL) return res;

    return SLCAN_IO_SUCCESS;
}

void slcan_serial_close(slcan_serial_handle_t serial_port)
{
    close(HANDLE_TO_SERIAL(serial_port));
}

int slcan_serial_read(slcan_serial_handle_t serial_port, void* data, size_t data_size)
{
    return read(HANDLE_TO_SERIAL(serial_port), data, data_size);
}

int slcan_serial_write(slcan_serial_handle_t serial_port, const void* data, size_t data_size)
{
    return write(HANDLE_TO_SERIAL(serial_port), data, data_size);
}

int slcan_serial_flush(slcan_serial_handle_t serial_port)
{
    int res;

    res = tcdrain(HANDLE_TO_SERIAL(serial_port));
    if(res == SLCAN_IO_FAIL) return res;

    res = tcflush(HANDLE_TO_SERIAL(serial_port), TCIOFLUSH);
    if(res == SLCAN_IO_FAIL) return res;

    return SLCAN_IO_SUCCESS;
}

// POLLIN
// POLLPRI
// POLLOUT
// POLLERR
// POLLHUP
// POLLNVAL
int slcan_serial_poll(slcan_serial_handle_t serial_port, int events, int* revents, int timeout)
{
    if(revents == NULL) return SLCAN_IO_FAIL;

    struct pollfd pfd;

    short in_events = 0;

    if(events & SLCAN_POLLIN) in_events |= POLLIN;
    if(events & SLCAN_POLLOUT) in_events |= POLLOUT;

    pfd.fd = HANDLE_TO_SERIAL(serial_port);
    pfd.events = in_events;
    pfd.revents = 0;

    int res = poll(&pfd, 1, timeout);
    if(res == SLCAN_IO_FAIL) return res;

    short out_events = 0;

    if(pfd.revents & POLLIN) out_events |= SLCAN_POLLIN;
    if(pfd.revents & POLLOUT) out_events |= SLCAN_POLLOUT;

    *revents = out_events;

    return SLCAN_IO_SUCCESS;
}

int slcan_serial_nbytes(slcan_serial_handle_t serial_port, size_t* size)
{
    if(size == NULL) return SLCAN_IO_FAIL;

    unsigned long len = 0;
    int res = 0;

    res = ioctl(HANDLE_TO_SERIAL(serial_port), TIOCINQ, &len);
    if(res == SLCAN_IO_FAIL) return res;

    *size = (size_t)len;

    return SLCAN_IO_SUCCESS;
}
