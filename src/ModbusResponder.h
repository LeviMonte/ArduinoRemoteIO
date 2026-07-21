#ifndef MODBUS_RESPONDER_H
#define MODBUS_RESPONDER_H

#include <Arduino.h>
#include <Client.h>

/*
  ArduinoRemoteIO - ModbusResponder

  Minimal Modbus TCP responder implementing Read Holding Registers
  (function code 0x03), built directly on the Modbus Application
  Protocol spec (modbus.org).

  This is a thin wrapper around the original serveModbusRequest() logic (see project examples), where
  the frame parsing itself is unchanged. The only difference from a
  single hardcoded sketch is that the register array, its length, and
  its base address are passed in instead of hardcoded, so the same
  class works for 1 register, 2, or N.
*/

class ModbusResponder {
  public:
    // regs        - pointer to YOUR holding register array (you own/update this array)
    // regCount    - number of registers in that array (NUM_HOLDING_REGS in your sketch)
    // regBaseAddr - Modbus address that regs[0] corresponds to (HOLDING_REG_ADDR)
    ModbusResponder(uint16_t* regs, uint16_t regCount, uint16_t regBaseAddr);

    // Call this once per loop iteration when a client is connected and
    // has data available - identical behavior to the original
    // serveModbusRequest(). Returns false if the connection should drop.
    bool serve(Client& client);

  private:
    uint16_t* _regs;
    uint16_t  _regCount;
    uint16_t  _regBaseAddr;
};

#endif
