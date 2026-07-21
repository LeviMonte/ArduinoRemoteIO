# ArduinoRemoteIO

A minimal Modbus TCP responder for Arduino, built to serve sensor/IO data
as holding registers to a PLC (tested against Allen Bradley PLCs) without the RAM and flash overhead of a full Modbus library.

## Advantages over ArduinoModbus

[ArduinoModbus](https://github.com/arduino-libraries/ArduinoModbus)
(LGPL-2.1, Copyright (c) 2018 Arduino SA) is a solid choice for most
Modbus projects as it supports multiple function codes and both RS485
and TCP. This project only ever needs one thing: Read Holding Registers
(FC03) served over TCP. On boards with very limited SRAM (like an Arduino nano/uno/etc.),
the full library's overhead (~700 B static RAM, ~300 B runtime heap, ~11 KB flash
in early testing here) wasn't worth carrying for that single code path,
so `ModbusResponder` implements just FC03 directly against the public
Modbus Application Protocol spec. It's a trade-off, but not necessarily a strict
improvement as you give up the library's flexibility for a smaller,
purpose-built footprint.

## Protocol reference
- Modbus Application Protocol V1.1b3 — https://www.modbus.org/specs.php
- MBAP framing + FC03 implemented in `src/ModbusResponder.cpp`

## Core API

```cpp
#define MODBUS_MAX_REGS 2       // size the buffer for your project
#include <ModbusResponder.h>

uint16_t holdingRegs[MODBUS_MAX_REGS] = { 0 };
ModbusResponder responder(holdingRegs, MODBUS_MAX_REGS, /* baseAddr */ 0);

// per client, per loop:
responder.serve(client);
```

`ModbusResponder` doesn't know or care where register values come from:
your sketch owns the array and updates it however it needs to (ADC reads,
calculations, etc.). This is what makes it reusable across
1-channel, 2-channel, or N-channel projects: only `MODBUS_MAX_REGS` and
your own update loop change.

## Examples
- [`examples/pressure_transducer_4_20mA`](examples/pressure_transducer) —
  two 4-20mA pressure transducers read via ADC + shunt resistor, served
  as holding registers, built for a JLab cryogenics PLC integration.
