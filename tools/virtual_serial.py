#!/usr/bin/env python3
"""Create a PTY and feed bytes so ./gateway <device> can read them."""

import argparse
import os
import pty
import subprocess
import sys
import termios
import time
import tty

BURSTS = [
    b"\x01\x03\x00\x00\x00\x02\xC4\x0B",
    b"hello",
    bytes(range(8)),
]


def send_bursts(master):
    for payload in BURSTS:
        os.write(master, payload)
        print("tx:", " ".join(f"{b:02X}" for b in payload), flush=True)
        time.sleep(0.5)  # > gateway VTIME (0.1s) so each burst becomes one frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", metavar="BIN", help="start gateway binary after creating PTY")
    args = parser.parse_args()

    master, slave = pty.openpty()
    slave_path = os.ttyname(slave)
    tty.setraw(slave, when=termios.TCSANOW)
    print(f"device: {slave_path}", flush=True)

    child = None
    if args.run:
        child = subprocess.Popen(
            [args.run, slave_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        time.sleep(0.4)
        if child.poll() is not None:
            sys.stderr.write(child.stderr.read())
            sys.exit(child.returncode or 1)
    else:
        print(f"in another terminal: ./gateway {slave_path}", flush=True)
        print("then press Enter here to send bytes...", flush=True)
        input()

    send_bursts(master)
    time.sleep(0.4)
    os.close(slave)
    os.close(master)

    if child is not None:
        child.send_signal(2)
        try:
            out, err = child.communicate(timeout=2)
        except subprocess.TimeoutExpired:
            child.kill()
            out, err = child.communicate()
        sys.stderr.write(err)
        sys.stdout.write(out)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(130)
