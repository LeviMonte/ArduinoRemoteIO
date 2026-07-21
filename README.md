# ModbusResponder

A minimal, dependency-free **Modbus TCP responder** for Arduino. It serves
sensor/IO data as **holding registers** to a PLC or SCADA master using a single
Modbus function code — Read Holding Registers (FC03) — without the RAM and flash
overhead of a full Modbus library.

Originally built for an Allen-Bradley PLC integration on my JLab SRGS
project, where every byte (2048 total) of SRAM on an AVR board mattered.

- **Header-only** — nothing to compile separately, no dependencies beyond the
  Arduino core.
- **Transport-agnostic** — works with any object implementing the Arduino
  `Client` interface (Ethernet, WiFi, etc.).
- **You own the data** — the responder reads from a register array that your
  sketch fills however it likes (ADC reads, calculations, other sensors).

## Installation

**Library Manager (recommended):** in the Arduino IDE, open
**Sketch → Include Library → Manage Libraries…**, search for `ModbusResponder`,
and click **Install**.

**Manual:** download this repository as a ZIP and use
**Sketch → Include Library → Add .ZIP Library…**.

## Quick start

```cpp
#define MODBUS_MAX_REGS 2        // size the buffer for your project
#include <ModbusResponder.h>

uint16_t holdingRegs[MODBUS_MAX_REGS] = { 0 };
ModbusResponder responder(holdingRegs, MODBUS_MAX_REGS, /* baseAddr */ 0);

// In your loop, when a client has data available:
responder.serve(client);         // client is any Arduino Client (e.g. EthernetClient)
```

`ModbusResponder` doesn't know or care where register values come from: your
sketch owns the array and updates it however it needs to. This is what makes it
reusable across 1-channel, 2-channel, or N-channel projects — only
`MODBUS_MAX_REGS` and your own update loop change.

## API

```cpp
ModbusResponder(uint16_t* regs, uint16_t regCount, uint16_t regBaseAddr);
```

| Parameter     | Meaning                                                        |
|---------------|----------------------------------------------------------------|
| `regs`        | Pointer to your holding-register array (you own and update it). |
| `regCount`    | Number of registers in that array (clamped to `MODBUS_MAX_REGS`). |
| `regBaseAddr` | Modbus address that `regs[0]` corresponds to.                   |

```cpp
bool serve(Client& client);
```

Handle one request from a connected client. Call it once per loop iteration when
the client has data available. Returns `false` if the connection should be
dropped (framing error, short read, or closed socket).

### Sizing the buffer

Define `MODBUS_MAX_REGS` **before** including the header to size the internal I/O
buffer. It defaults to `125` (the FC03 spec maximum) if left undefined, which
reserves more RAM than most projects need — so set it to your channel count:

```cpp
#define MODBUS_MAX_REGS 2
#include <ModbusResponder.h>
```

Because a `#define` in your sketch can only reach code in a header (not a
separately compiled `.cpp`), this library is intentionally header-only so the
buffer is sized at your include site.

## Examples

- **[BasicHoldingRegisters](examples/BasicHoldingRegisters)** — the smallest
  possible sketch: serves two registers with demo values over Ethernet. Start
  here to learn the API.
- **[example_transducer](examples/example_transducer)** — a real deployment
  using the library: two 4-20mA pressure transducers read via ADC + shunt
  resistor, filtered, and served as holding registers to a cryogenics PLC.
- **[real_transducer](examples/real_transducer)** — the original, hand-rolled
  version this library was distilled from. It handles the Modbus framing and
  FC03 reply *inline*, without the library, so you can see exactly what
  `ModbusResponder::serve()` does internally.

## Scope and limitations

This library implements only **Read Holding Registers (FC03) over Modbus TCP**.
Any other function code returns an "illegal function" exception (0x01), and
out-of-range requests return "illegal data address" (0x02). If you need write
support, multiple function codes, or Modbus RTU/RS-485, use the fuller-featured
[ArduinoModbus](https://github.com/arduino-libraries/ArduinoModbus) library
instead. This project deliberately trades that flexibility for a tiny footprint.

## Protocol reference

- Modbus Application Protocol V1.1b3 — https://www.modbus.org/specs.php
- MBAP framing + FC03 are implemented inline in
  [`src/ModbusResponder.h`](src/ModbusResponder.h).

## License

Apache License 2.0. See [LICENSE](LICENSE).
