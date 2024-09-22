#include "slcan_serial_io_cygwin.h"
#include "slcan_serial_io.h"
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/ioctl.h>


int slcan_serial_open(const char* serial_port_name)
{
    int s = open(serial_port_name, O_RDWR);

    return s;
}

int slcan_serial_configure(int serial_port, slcan_port_conf_t* conf)
{
    static int slcan_port_bauds_impl[] = {
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

    res = tcgetattr(serial_port, &tty);
    if(res == -1) return res;


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
    res = tcsetattr(serial_port, TCSANOW, &tty);
    if(res == -1) return res;


    // set non blocking io.
    int flags = fcntl(serial_port, F_GETFL, 0);
    if(flags == -1) return flags;

    res = fcntl(serial_port, F_SETFL, flags | O_NONBLOCK);
    if(res == -1) return res;

    return 0;
}

void slcan_serial_close(int serial_port)
{
    close(serial_port);
}

int slcan_serial_read(int serial_port, void* data, size_t data_size)
{
    return read(serial_port, data, data_size);
}

int slcan_serial_write(int serial_port, const void* data, size_t data_size)
{
    return write(serial_port, data, data_size);
}

int slcan_serial_flush(int serial_port)
{
    int res;

    res = tcflush(serial_port, TCIOFLUSH);
    if(res == -1) return res;

    return 0;
}

// POLLIN
// POLLPRI
// POLLOUT
// POLLERR
// POLLHUP
// POLLNVAL
int slcan_serial_poll(int serial_port, int events, int* revents, int timeout)
{
    if(revents == NULL){
        errno = EINVAL;
        return -1;
    }

    struct pollfd pfd;

    short in_events = 0;

    if(events & SLCAN_POLLIN) in_events |= POLLIN;
    if(events & SLCAN_POLLOUT) in_events |= POLLOUT;

    pfd.fd = serial_port;
    pfd.events = in_events;
    pfd.revents = 0;

    int res = poll(&pfd, 1, timeout);
    if(res == -1) return res;

    short out_events = 0;

    if(pfd.revents & POLLIN) out_events |= SLCAN_POLLIN;
    if(pfd.revents & POLLOUT) out_events |= SLCAN_POLLOUT;

    *revents = out_events;

    return 0;
}

int slcan_serial_nbytes(int serial_port, size_t* size)
{
    if(size == NULL){
        errno = EINVAL;
        return -1;
    }

    unsigned long len = 0;
    int res = 0;

    res = ioctl(serial_port, TIOCINQ, &len);
    if(res == -1) return res;

    *size = (size_t)len;

    return 0;
}


void slcan_serial_io_cygwin_init(slcan_serial_io_t* io)
{
    io->open = slcan_serial_open;
    io->configure = slcan_serial_configure;
    io->close = slcan_serial_close;
    io->read = slcan_serial_read;
    io->write = slcan_serial_write;
    io->flush = slcan_serial_flush;
    io->poll = slcan_serial_poll;
    io->nbytes = slcan_serial_nbytes;
}
