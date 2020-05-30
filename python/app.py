#!/usr/bin/env python3

# Copyright 2020 Daniel Koch
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

"""Example Python script demonstrating serial interoperability with Arduino

This script uses cppimport to import the serial protocol ported to
Python using pybind11. Serial communication uses the pyserial module,
and reading from the serial port is carried out asynchronously in a
separate thread. In production, a more robust producer-consumer
threading model should most likely be used.
"""

__author__ = "Daniel Koch"
__email__ = "daniel.p.koch@gmail.com"

__copyright__ = "Copyright 2020 Daniel Koch"
__license__ = "MIT"

__version__ = "0.1"

import cppimport.import_hook
import protocol

import argparse
import serial
import signal
import sys
import threading
import time

def handleHeartbeat(msg):
    print(f"Got heartbeat message: {msg.count}")

def handleResponse(msg):
    print(f"Got response: {msg.a} + {msg.b} = {msg.c}")

def serialReceive(ser, event):
    parser = protocol.Parser()
    msg = protocol.Message()

    while not event.is_set():
        byte = ser.read(size=1)
        if len(byte) > 0 and parser.parseByte(byte[0], msg):
            if msg.id == protocol.Heartbeat.id():
                handleHeartbeat(msg.getHeartbeat())
            elif msg.id == protocol.Response.id():
                handleResponse(msg.getResponse())


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Arduino-Python serial interoperability')
    parser.add_argument('port', type=str, help="The serial port device, e.g. '/dev/ttyACM0'")
    args = parser.parse_args()

    request = protocol.Request()
    request.b = 1

    with serial.Serial(port=args.port, baudrate=115200, timeout=1) as ser:
        # set up threading for rx thread
        shutdownEvent = threading.Event()
        rxThread = threading.Thread(target=serialReceive, args=(ser, shutdownEvent))

        # register custom SIGINT (Ctrl+C) handler to gracefully shut down
        def signal_handler(sig, frame):
            shutdownEvent.set()
            rxThread.join()
            sys.exit(0)

        signal.signal(signal.SIGINT, signal_handler)

        print("Press Ctrl+C to stop...")
        rxThread.start()
        while True:
            ser.write(protocol.getBytes(request))
            request.a += 1
            request.b += 1

            time.sleep(1)
