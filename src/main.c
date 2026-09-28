#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

/* Later: take device from argv or config. */
#define DEVICE "/tmp/gateway-pty"

int main(void)
{
    unsigned char buf[256];
    struct termios tio;
    ssize_t n, i;
    int fd;

    fd = open(DEVICE, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (tcgetattr(fd, &tio) < 0) {
        perror("tcgetattr");
        return 1;
    }
    cfmakeraw(&tio);
    cfsetispeed(&tio, B9600);
    cfsetospeed(&tio, B9600);
    tio.c_cflag |= (CLOCAL | CREAD);
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        perror("tcsetattr");
        return 1;
    }

    for (;;) {
        n = read(fd, buf, sizeof buf);
        if (n <= 0)
            break;
        for (i = 0; i < n; i++)
            printf("%02X ", buf[i]);
        printf("\n");
        fflush(stdout);
    }

    close(fd);
    return 0;
}
