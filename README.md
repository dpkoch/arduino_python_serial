# Arduino-Serial Python

This project shows an example implementation for defining a serial protocol that can be used to communicate between an Arduino and a Python script. The protocol is defined in C++, and is then made available to Python as a module using [pybind11](https://github.com/pybind/pybind11).

## Protocol Definition

The protocol is defined in `arduino/protocol.h`. This header is a somewhat stripped down version of the [serial_factory](https://github.com/dpkoch/serial_factory) project, to compensate for the limited C++ standard library implementation available for Arduino.

This header file needs to be located in the same directory as the Arduino sketch in order for the Arduino IDE/compiler to find it. Message types are defined as structs, and a few additional definitions need to be made for each message defined.

The section of the header that requires modification is marked, and it should be fairly straightforward to follow the example implementation by searching for all instances of the example message types. The example implementation defines three messages: `Heartbeat`, `Request`, and `Response`.

## Arduino Sketch

A simple example Arduino sketch is provided at `arduino/arduino.ino`. This sketch sends a `Heartbeat` message once per second, and listens asynchronously for `Request` messages, which it handles by sending a `Response` message in reply.

## Python Script

The protocol definition is made available as a Python module using pybind11. The code to do this is contained in `python/protocol.cpp`. Again, the section of the file that requires modification is marked, and it should be fairly straightforward to follow the example implementation by searching for all instances of the example message types.

An example Python script is located at`python/app.py`, and a `requirements.txt` file is provided with the necessary dependencies. The script imports the `protocol` module produced by pybind11 using [cppimport](https://github.com/tbenthompson/cppimport) (with the Python import hook method), which automatically compiles the module as needed when the script is run.

The example Python script sends a `Request` message once per second. It also handles incoming serial data on a separate thread in an aynchronous paradigm. The threading module used here is very simple; in production a more robust queue-based producer-consumer threading model would likely be a better choice.