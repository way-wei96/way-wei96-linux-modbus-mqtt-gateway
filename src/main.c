#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/* 把常见波特率数字映射成 termios 常量；不支持则返回 (speed_t)-1。 */
static speed_t baud_to_speed(int baud)
{
    switch (baud) {
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return (speed_t)-1;
    }
}

/* Modbus RTU CRC16：初值 0xFFFF，多项式 0xA001（反射形式）。 */
static uint16_t modbus_crc16(const unsigned char *data, size_t len)
{
    uint16_t crc = 0xFFFF;
    size_t i;
    int b;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (b = 0; b < 8; b++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

static void dump_frame(const unsigned char *frame, size_t frame_len)
{
    size_t j;
    uint16_t got;
    uint16_t expect;

    for (j = 0; j < frame_len; j++)
        printf("%02X ", frame[j]);

    /* RTU 至少：地址 + 功能码 + CRC_L + CRC_H */
    if (frame_len < 4) {
        printf("  [too short]\n");
        return;
    }

    got = (uint16_t)frame[frame_len - 2]
        | ((uint16_t)frame[frame_len - 1] << 8);
    expect = modbus_crc16(frame, frame_len - 2);
    if (got == expect)
        printf("  [crc ok]\n");
    else
        printf("  [crc bad expect=%02X %02X]\n",
               (unsigned)(expect & 0xFF),
               (unsigned)((expect >> 8) & 0xFF));
}

int main(int argc, char **argv)
{
    unsigned char buf[256];
    unsigned char frame[256];
    size_t frame_len = 0;
    struct termios tio;
    ssize_t n, i;
    int fd;
    int baud = 9600;
    speed_t speed;
    const char *device;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <device> [baud]\n", argv[0]);
        return 1;
    }
    device = argv[1];
    if (argc == 3)
        baud = atoi(argv[2]);

    speed = baud_to_speed(baud);
    if (speed == (speed_t)-1) {
        fprintf(stderr, "unsupported baud: %d\n", baud);
        fprintf(stderr, "supported: 9600 19200 38400 57600 115200\n");
        return 1;
    }

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
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);
    /* CLOCAL 忽略 modem 线；CREAD 允许接收。 */
    tio.c_cflag |= (CLOCAL | CREAD);
    /*
     * VMIN=0, VTIME=1：最多等 0.1s。
     * 超时且缓冲非空 → 视为一帧结束（RTU 式空闲间隔）。
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
            /* 空闲超时：整帧打印，并做 CRC 校验。 */
            if (frame_len > 0) {
                dump_frame(frame, frame_len);
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
