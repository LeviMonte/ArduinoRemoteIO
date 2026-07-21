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
  defaults to 125, which costs more RAM than most projects need (which
  this library is trying to save!).

  IMPORTANT - why this is header-only (no separate .cpp):
  A library .cpp file is compiled as its own translation unit, entirely
  separate from your sketch. A #define in your .ino can never reach code
  compiled inside a .cpp, no matter where in the .ino it appears or how
  the includes are ordered. Keeping serve() defined here, inline, means
  the buffer below is sized using whatever MODBUS_MAX_REGS is set to at
  *your* include site - which is the only way the customization this
  comment describes actually works. Do not split this back into a .cpp.
*/
#ifndef MODBUS_MAX_REGS
#define MODBUS_MAX_REGS 125
#endif

class ModbusResponder {
  public:
    // regs        - pointer to YOUR holding register array (you own/update this array)
    // regCount    - number of registers in that array (must be <= MODBUS_MAX_REGS)
    // regBaseAddr - Modbus address that regs[0] corresponds to
    ModbusResponder(uint16_t* regs, uint16_t regCount, uint16_t regBaseAddr)
      : _regs(regs),
        _regCount(regCount > MODBUS_MAX_REGS ? MODBUS_MAX_REGS : regCount),
        _regBaseAddr(regBaseAddr)
    {
      // Defensive clamp: if regCount is ever larger than MODBUS_MAX_REGS
      // (e.g. someone bumps a channel count without also bumping the
      // buffer sizing constant), we clamp here rather than let serve()
      // write past the end of its own read/write buffer.
    }

    // Call once per loop iteration when a client has data available.
    // Returns false if the connection should be dropped.
    bool serve(Client& client) {
      uint8_t buf[9 + 2 * MODBUS_MAX_REGS]; // sized off MODBUS_MAX_REGS at *this* include site

      if (client.readBytes(buf, 7) != 7) return false;

      uint16_t protoId = ((uint16_t)buf[2] << 8) | buf[3];
      uint16_t length  = ((uint16_t)buf[4] << 8) | buf[5];
      if (protoId != 0 || length < 2 || length > 255) return false;

      uint8_t pduLen = length - 1;
      const uint8_t maxPduCapacity = sizeof(buf) - 7;
      uint8_t toRead = pduLen > maxPduCapacity ? maxPduCapacity : pduLen;

      if (client.readBytes(buf + 7, toRead) != toRead) return false;
      for (uint8_t i = toRead; i < pduLen; i++) {
        if (client.read() < 0) return false; // drain the rest so the stream stays in sync
      }

      uint8_t fc = buf[7];
      uint8_t pduLenResp;

      if (fc == 0x03 && pduLen == 5) {
        uint16_t addr = ((uint16_t)buf[8] << 8) | buf[9];
        uint16_t qty  = ((uint16_t)buf[10] << 8) | buf[11];

        if (qty == 0 || addr < _regBaseAddr ||
            (uint32_t)addr + qty > (uint32_t)_regBaseAddr + _regCount) {
          buf[7] = fc | 0x80;
          buf[8] = 0x02; // illegal data address
          pduLenResp = 2;
        } else {
          buf[8] = qty * 2;
          uint8_t p = 9;
          for (uint16_t i = 0; i < qty; i++) {
            uint16_t v = _regs[addr - _regBaseAddr + i];
            buf[p++] = v >> 8;
            buf[p++] = v & 0xFF;
          }
          pduLenResp = 2 + qty * 2;
        }
      } else {
        buf[7] = fc | 0x80;
        buf[8] = 0x01; // illegal function
        pduLenResp = 2;
      }

      uint16_t mbapLength = pduLenResp + 1;
      buf[4] = mbapLength >> 8;
      buf[5] = mbapLength & 0xFF;

      client.write(buf, 7 + pduLenResp);
      return true;
    }

  private:
    uint16_t* _regs;
    uint16_t  _regCount;
    uint16_t  _regBaseAddr;
};

#endif
