/*
 * Copyright 2020 Daniel Koch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file arduino.ino
 * @author Daniel Koch <daniel.p.koch@gmail.com>
 * @brief Example Arduino sketch demonstrating serial protocol interoperability with Python
 * @version 0.1
 * @date 2020-05-29
 *
 * @copyright Copyright (c) 2020 Daniel Koch.
 * This project is released under the MIT license.
 */

#include "protocol.h"

#include <stddef.h>
#include <stdint.h>

protocol::Message msg;
protocol::Parser parser;
uint8_t serial_buffer[sizeof(protocol::Message)];

protocol::Heartbeat heartbeat;

constexpr unsigned long HEARTBEAT_PERIOD_MS = 1000;
unsigned long last_heartbeat_ms = 0;

volatile int bytes_received = 0;
volatile bool new_request = false;
volatile protocol::Request request;

void handleRequest()
{
  protocol::Response response;
  response.a = request.a;
  response.b = request.b;
  response.c = request.a + request.b;

  size_t len = protocol::sendToBuffer(response, serial_buffer);
  Serial.write(serial_buffer, len);
}

void sendHeartbeat()
{
  heartbeat.count = bytes_received;
  size_t len = sendToBuffer(heartbeat, serial_buffer);
  Serial.write(serial_buffer, len);
}

void setup()
{
  Serial.begin(115200);
}

void loop()
{
  if (millis() > last_heartbeat_ms + HEARTBEAT_PERIOD_MS)
  {
    sendHeartbeat();
    last_heartbeat_ms = millis();
  }

  if (new_request)
  {
    handleRequest();
    new_request = false;
  }
}

void serialEvent()
{
  while (Serial.available())
  {
    ++bytes_received;
    if (parser.parseByte(static_cast<uint8_t>(Serial.read()), msg))
    {
      switch (msg.id)
      {
      case protocol::getID<protocol::Request>():
        request = msg.payload.message.request;
        new_request = true;
        break;
      }
    }
  }
}
