#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    unsigned char buf[256];
    unsigned char frame[256];
    size_t frame_len = 0;
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
    /*
     * VMIN=0, VTIME=1：最多等 0.1s。
     * 超时且缓冲非空 → 视为一帧结束（RTU 式空闲间隔，尚未解析 Modbus）。
     */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        perror("tcsetattr");
        return 1;
    }

    for (;;) {
        n = read(fd, buf, sizeof buf);
        if (n > 0) {
            /* 有数据：先攒着，不急着按 read 边界打印。 */
            for (i = 0; i < n; i++) {
                if (frame_len >= sizeof frame) {
                    fprintf(stderr, "frame overflow, drop %zu bytes\n", frame_len);
                    frame_len = 0;
                }
                frame[frame_len++] = buf[i];
            }
            continue;
        }
        if (n == 0) {
            /* 空闲超时：把已攒字节当成一帧打出。 */
            if (frame_len > 0) {
                size_t j;

                for (j = 0; j < frame_len; j++)
                    printf("%02X ", frame[j]);
                printf("\n");
                fflush(stdout);
                frame_len = 0;
            }
            continue;
        }
        if (errno == EINTR)
            continue;
        perror("read");
        break;
    }

    close(fd);
    return 0;
}
