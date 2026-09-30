#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    unsigned char buf[256];
    struct termios tio;
    ssize_t n, i;
    int fd;
    const char *device;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        return 1;
    }
    device = argv[1];

    /* O_NOCTTY：别把该串口收编成控制终端，避免设备字节被当成 Ctrl-C。 */
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (tcgetattr(fd, &tio) < 0) {
        perror("tcgetattr");
        return 1;
    }
    /* raw：要原始字节流，不要终端行编辑/回显等加工。 */
    cfmakeraw(&tio);
    cfsetispeed(&tio, B9600);
    cfsetospeed(&tio, B9600);
    /* CLOCAL 忽略 modem 线；CREAD 允许接收。 */
    tio.c_cflag |= (CLOCAL | CREAD);
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        perror("tcsetattr");
        return 1;
    }

    for (;;) {
        /* 一次未必读完所有到达数据；n 是本轮实际字节数。 */
        n = read(fd, buf, sizeof buf);
        if (n <= 0)
            break;
        for (i = 0; i < n; i++)
            printf("%02X ", buf[i]);
        printf("\n");
        fflush(stdout); /* 尽快显示，便于盯串口实时输出。 */
    }

    close(fd);
    return 0;
}
