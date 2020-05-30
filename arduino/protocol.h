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
 * @file protocol.h
 * @author Daniel Koch <daniel.p.koch@gmail.com>
 * @brief Serial protocol definition for Arduino-Python interoperability
 * @version 0.1
 * @date 2020-05-29
 *
 * @copyright Copyright (c) 2020 Daniel Koch.
 * This project is released under the MIT license.
 *
 * This file defines the serial protocol. It is used directly by the Arduino
 * sketch, and is made available as a Python module using pybind11 in
 * protocol.cpp.
 *
 * This file defines the custom messages, as well as additional supporting
 * functionality such as sending message to a buffer and parsing incoming
 * messages.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace protocol
{

#define SERIAL_MESSAGE __attribute__((packed))

// source: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html#gab27eaaef6d7fd096bd7d57bf3f9ba083
inline uint8_t update_crc(uint8_t current, uint8_t byte)
{
  uint8_t data = current ^ byte;

  for (uint8_t i = 0; i < 8; i++)
  {
    if ((data & 0x80) != 0)
    {
      data <<= 1;
      data ^= 0x07;
    }
    else
    {
      data <<= 1;
    }
  }
  return data;
}

// get max size of a list of types
template <typename T>
constexpr size_t maxSizeOf()
{
  return sizeof(T);
}

template <typename T1, typename T2, typename... Ts>
constexpr size_t maxSizeOf()
{
  return sizeof(T1) > maxSizeOf<T2, Ts...>() ? sizeof(T1) : maxSizeOf<T2, Ts...>();
}

// struct for storing info about list of message types
template <typename... Ts>
struct InfoTemplate
{
  static constexpr uint8_t maxPayloadSize = maxSizeOf<Ts...>();
};

constexpr uint8_t START_BYTE = 0xA5;

//=============================================================================
// BEGIN CUSTOMIZATION AREA
//=============================================================================

struct SERIAL_MESSAGE Heartbeat
{
  uint32_t count;
};

struct SERIAL_MESSAGE Request
{
  int32_t a;
  int32_t b;
};

struct SERIAL_MESSAGE Response
{
  int32_t a;
  int32_t b;
  int32_t c;
};

enum class MessageID : uint8_t
{
  HEARTBEAT,
  REQUEST,
  RESPONSE
};

template <typename T> constexpr MessageID getID() = delete;
template <> constexpr MessageID getID<Heartbeat>() { return MessageID::HEARTBEAT; }
template <> constexpr MessageID getID<Request>()   { return MessageID::REQUEST; }
template <> constexpr MessageID getID<Response>()  { return MessageID::RESPONSE; }

using Info = InfoTemplate<Heartbeat, Request, Response>;

struct Message
{
  uint8_t start_byte = 0xA5;
  MessageID id;
  uint8_t len;
  union {
    uint8_t buffer[Info::maxPayloadSize];
    union {
      Heartbeat heartbeat;
      Request request;
      Response response;
    } message;
  } payload;
  uint8_t crc;

  uint8_t getID() { return static_cast<uint8_t>(id); }
  const Heartbeat& getHeartbeat() const { return payload.message.heartbeat; }
  const Request& getRequest() const { return payload.message.request; }
  const Response& getResponse() const { return payload.message.response; }
};

//=============================================================================
// END CUSTOMIZATION AREA
//=============================================================================

template <typename T>
size_t sendToBuffer(const T& msg, uint8_t* buffer)
{
  buffer[0] = START_BYTE;
  buffer[1] = static_cast<uint8_t>(getID<T>());
  buffer[2] = sizeof(T);
  memcpy(buffer + 3, &msg, sizeof(T));

  size_t crc_index = 3 + sizeof(T);
  uint8_t crc = 0;
  for (size_t i = 0; i < crc_index; i++)
  {
    crc = update_crc(crc, buffer[i]);
  }
  buffer[crc_index] = crc;

  return crc_index + 1;
}

class Parser
{
public:
  bool parseByte(uint8_t byte, Message &msg)
  {
    bool got_message = false;

    switch (parse_state)
    {
    case ParseState::IDLE:
      if (byte == START_BYTE)
      {
        msg_buffer.crc = update_crc(msg_buffer.crc, byte);
        parse_state = ParseState::GOT_START_BYTE;
      }
      break;
    case ParseState::GOT_START_BYTE:
      msg_buffer.id = static_cast<MessageID>(byte);
      msg_buffer.crc = update_crc(msg_buffer.crc, byte);
      parse_state = ParseState::GOT_ID;
      break;
    case ParseState::GOT_ID:
      msg_buffer.len = byte;
      msg_buffer.crc = update_crc(msg_buffer.crc, byte);
      if (msg_buffer.len > 0)
      {
        parse_state = ParseState::GOT_LENGTH;
        payload_bytes_received = 0;
      }
      else
      {
        parse_state = ParseState::GOT_PAYLOAD;
      }
      break;
    case ParseState::GOT_LENGTH:
      msg_buffer.payload.buffer[payload_bytes_received++] = byte;
      msg_buffer.crc = update_crc(msg_buffer.crc, byte);
      if (payload_bytes_received >= msg_buffer.len)
      {
        parse_state = ParseState::GOT_PAYLOAD;
      }
      break;
    case ParseState::GOT_PAYLOAD:
      if (byte == msg_buffer.crc)
      {
        got_message = true;
        memcpy(reinterpret_cast<void *>(&msg), reinterpret_cast<const void *>(&msg_buffer), sizeof(msg));
      }
      msg_buffer.crc = 0;
      parse_state = ParseState::IDLE;
    }

    return got_message;
  }

private:
  enum class ParseState
  {
    IDLE,
    GOT_START_BYTE,
    GOT_ID,
    GOT_LENGTH,
    GOT_PAYLOAD
  };

  ParseState parse_state = ParseState::IDLE;
  size_t payload_bytes_received = 0;
  Message msg_buffer;
};

} // namespace protocol

#endif // PROTOCOL_H
