# way-wei96-linux-modbus-mqtt-gateway

Linux industrial gateway in C. This slice reads a serial port, assembles RTU-style frames, and checks Modbus CRC16.

## Build

```text
cc -Wall -Wextra -O0 -g -o gateway src/main.c
```

## Run

```text
./gateway <device> [baud]
```

`baud` defaults to 9600. Supported: 9600, 19200, 38400, 57600, 115200.

Without real hardware (Python PTY):

```text
python3 tools/virtual_serial.py --run ./gateway
```

Or two terminals: run `python3 tools/virtual_serial.py`, then start `./gateway` with the printed device path, then press Enter in the script.

Real serial example:

```text
./gateway /dev/tty.usbserial 9600
```

`read` uses a short termios timeout (VMIN=0, VTIME=1). Bytes are buffered into a frame after an idle gap, checked with Modbus RTU CRC16, then parsed as a slave response: address, function code, and for FC03 the byte count.

The demo script feeds a sample FC03 response (not a master request). No register-value decode or MQTT yet.
