# way-wei96-linux-modbus-mqtt-gateway

Linux industrial gateway in C. This slice only reads a serial port and hex-dumps bytes.

Device path is hardcoded in `src/main.c` as `/tmp/gateway-pty` for now.
Point that path at a real tty first, for example:

```text
ln -sf /dev/tty.usbserial /tmp/gateway-pty
```

Build and run with `cc`:

```text
cc -Wall -Wextra -O0 -g -o gateway src/main.c
./gateway
```

No Modbus or MQTT yet.
