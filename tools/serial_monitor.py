#!/usr/bin/env python3
"""Bounded USB CDC capture, no third-party dependencies (Linux/macOS)."""
import argparse
import os
import select
import termios
import time

p = argparse.ArgumentParser()
p.add_argument('--port', default='/dev/ttyACM0')
p.add_argument('--seconds', type=float, default=15)
p.add_argument('--send', default='')
p.add_argument('--output')
a = p.parse_args()
fd = os.open(a.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
attrs = termios.tcgetattr(fd)
attrs[0] = attrs[1] = attrs[3] = 0
attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
attrs[4] = attrs[5] = termios.B115200
attrs[6][termios.VMIN] = 0
attrs[6][termios.VTIME] = 0
termios.tcsetattr(fd, termios.TCSANOW, attrs)
log = open(a.output, 'w') if a.output else None
try:
    if a.send:
        os.write(fd, a.send.encode('ascii'))
    end = time.monotonic() + a.seconds
    while time.monotonic() < end:
        if select.select([fd], [], [], min(.5, max(0, end-time.monotonic())))[0]:
            try:
                data = os.read(fd, 8192)
            except BlockingIOError:
                continue
            if not data:
                break
            text = data.decode('utf-8', errors='replace')
            print(text, end='', flush=True)
            if log:
                log.write(text)
                log.flush()
finally:
    os.close(fd)
    if log:
        log.close()
