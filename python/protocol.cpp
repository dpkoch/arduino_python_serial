/*cppimport
<%
setup_pybind11(cfg)
%>
*/

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
 * @file protocol.cpp
 * @author Daniel Koch <daniel.p.koch@gmail.com>
 * @brief Serial protocol pybind11 implementation for Arduino-Python interoperability
 * @version 0.1
 *
 * @copyright Copyright (c) 2020 Daniel Koch.
 * This project is released under the MIT license.
 *
 * https://github.com/dpkoch/arduino_python_serial
 *
 * This file uses pybind11 to make the serial protocol defined in protocol.h
 * available as a Python module.
 */

#include "../arduino/protocol.h"
#include <pybind11/pybind11.h>

namespace py = pybind11;

template <typename T>
py::bytes getBytes(const T& msg)
{
  uint8_t buffer[sizeof(protocol::Message)];
  size_t len = protocol::sendToBuffer<T>(msg, buffer);
  return py::bytes(reinterpret_cast<const char *>(buffer), len);
}

template <typename T>
constexpr uint8_t getIntID() { return static_cast<uint8_t>(protocol::getID<T>()); }

PYBIND11_MODULE(protocol, m)
{
  //============================================================================
  // BEGIN CUSTOMIZATION AREA
  //============================================================================

  // custom messages
  py::class_<protocol::Heartbeat>(m, "Heartbeat")
    .def(py::init<>())
    .def_static("id", &getIntID<protocol::Heartbeat>)
    .def_readwrite("count", &protocol::Heartbeat::count);
  m.def("getBytes", (py::bytes (*)(const protocol::Heartbeat&)) &getBytes);

  py::class_<protocol::Request>(m, "Request")
    .def(py::init<>())
    .def_static("id", &getIntID<protocol::Request>)
    .def_readwrite("a", &protocol::Request::a)
    .def_readwrite("b", &protocol::Request::b);
  m.def("getBytes", (py::bytes (*)(const protocol::Request&)) &getBytes);

  py::class_<protocol::Response>(m, "Response")
    .def(py::init<>())
    .def_static("id", &getIntID<protocol::Response>)
    .def_readwrite("a", &protocol::Response::a)
    .def_readwrite("b", &protocol::Response::b)
    .def_readwrite("c", &protocol::Response::c);
  m.def("getBytes", (py::bytes (*)(const protocol::Response&)) &getBytes);

  // generic message
  py::class_<protocol::Message>(m, "Message")
    .def(py::init<>())
    .def_property_readonly("id", &protocol::Message::getID)
    .def("getHeartbeat", &protocol::Message::getHeartbeat)
    .def("getRequest", &protocol::Message::getRequest)
    .def("getResponse", &protocol::Message::getResponse);

  //============================================================================
  // END CUSTOMIZATION AREA
  //============================================================================

  py::class_<protocol::Parser>(m, "Parser")
    .def(py::init<>())
    .def("parseByte", &protocol::Parser::parseByte);
}
