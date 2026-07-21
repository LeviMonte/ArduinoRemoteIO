# Setup

## Install as an Arduino library
1. Download this repo as a ZIP (Code → Download ZIP), or clone it.
2. In the Arduino IDE: **Sketch → Include Library → Add .ZIP Library...**
   and select the downloaded ZIP.
   - Alternatively, unzip and place the whole `ArduinoRemoteIO` folder
     directly in your `Arduino/libraries/` directory, then restart the IDE.
3. Confirm it installed: **File → Examples → ArduinoRemoteIO** should
   list `pressure_transducer_4_20mA`.

## Try the example
1. Open **File → Examples → ArduinoRemoteIO → example_transducer**.
2. Edit `constants.h` for your setup:
   - `mac[]` — unique MAC per device on your subnet
   - `staticIP` (in the `.ino`) — match your PLC's subnet
   - `NUM_HOLDING_REGS` / `MODBUS_MAX_REGS` — number of channels
   - `PRESSURE_PIN`, `DP_PIN` — your analog input pins
   - `VREF_MV`, `ADC_MAX`, `SHUNT_OHMS` — your board/hardware values
3. Upload to your board.
4. Point your PLC's Modbus TCP client (e.g. an MSG instruction in
   Studio 5000) at the board's static IP, port 502, function code 3
   (Read Holding Registers), starting at `HOLDING_REG_ADDR`.

## Build your own sketch (not just the example)
1. `#define MODBUS_MAX_REGS <n>` before including the library, where
   `<n>` is however many registers you need.
2. `#include <ModbusResponder.h>`
3. Declare `uint16_t holdingRegs[<n>]` and a `ModbusResponder` pointed
   at it.
4. Fill `holdingRegs[]` however you want (sensor reads, math, whatever) —
   the library doesn't care what's in it, just that it's there.
5. Call `responder.serve(client)` once per loop iteration when a client
   has data ready.
