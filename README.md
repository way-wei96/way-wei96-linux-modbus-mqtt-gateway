# way-wei96-linux-modbus-mqtt-gateway

Linux industrial gateway in C. This slice only reads a serial port and hex-dumps bytes.

## Build

```text
cc -Wall -Wextra -O0 -g -o gateway src/main.c
```

## Run

```text
./gateway <device>
```

Without real hardware (Python PTY):

```text
python3 tools/virtual_serial.py --run ./gateway
```

Or two terminals: run `python3 tools/virtual_serial.py`, then start `./gateway` with the printed device path, then press Enter in the script.

Real serial example:

```text
./gateway /dev/tty.usbserial
```

`read` uses a short termios timeout (VMIN=0, VTIME=1). Bytes are buffered and printed as one line after an idle gap — a simple RTU-style frame boundary, not Modbus parsing yet.

No Modbus or MQTT yet.
