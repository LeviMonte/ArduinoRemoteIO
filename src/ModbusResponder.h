#ifndef MODBUS_RESPONDER_H
#define MODBUS_RESPONDER_H

#include <Arduino.h>
#include <Client.h>

/*
  ArduinoRemoteIO - ModbusResponder

  Minimal Modbus TCP responder implementing Read Holding Registers
  (function code 0x03), built directly on the Modbus Application
  Protocol spec (modbus.org).

  CUSTOMIZE: define MODBUS_MAX_REGS *BEFORE* including this header
  to size the internal buffer for your project (e.g. 2 for a two-channel
  sensor, up to 125 - the spec max for FC03). If left undefined, it
  defaults to 125, which costs more RAM than MOST projects need (which this project is trying to save!).
  This is the main knob for keeping the small-footprint benefit.
*/
#ifndef MODBUS_MAX_REGS
#define MODBUS_MAX_REGS 125
#endif

class ModbusResponder {
  public:
    // regs        - pointer to YOUR holding register array (you own/update this array)
    // regCount    - number of registers in that array (must be <= MODBUS_MAX_REGS)
    // regBaseAddr - Modbus address that regs[0] corresponds to
    ModbusResponder(uint16_t* regs, uint16_t regCount, uint16_t regBaseAddr);

    // Call once per loop iteration when a client has data available.
    // Returns false if the connection should be dropped.
    bool serve(Client& client);

  private:
    uint16_t* _regs;
    uint16_t  _regCount;
    uint16_t  _regBaseAddr;
};

#endif
