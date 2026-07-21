#include "ModbusResponder.h"

ModbusResponder::ModbusResponder(uint16_t* regs, uint16_t regCount, uint16_t regBaseAddr)
  : _regs(regs), _regCount(regCount), _regBaseAddr(regBaseAddr)
{
}

bool ModbusResponder::serve(Client& client) {
  uint8_t buf[9 + 2 * MODBUS_MAX_REGS]; // sized off your MODBUS_MAX_REGS, not the spec max

  if (client.readBytes(buf, 7) != 7) return false;

  uint16_t protoId = ((uint16_t)buf[2] << 8) | buf[3];
  uint16_t length  = ((uint16_t)buf[4] << 8) | buf[5];
  if (protoId != 0 || length < 2 || length > 255) return false;

  uint8_t pduLen = length - 1;
  const uint8_t maxPduCapacity = sizeof(buf) - 7;
  uint8_t toRead = pduLen > maxPduCapacity ? maxPduCapacity : pduLen;
  if (client.readBytes(buf + 7, toRead) != toRead) return false;
  for (uint8_t i = toRead; i < pduLen; i++) {
    if (client.read() < 0) return false;
  }

  uint8_t fc = buf[7];
  uint8_t pduLenResp;

  if (fc == 0x03 && pduLen == 5) {
    uint16_t addr = ((uint16_t)buf[8] << 8) | buf[9];
    uint16_t qty  = ((uint16_t)buf[10] << 8) | buf[11];

    if (qty == 0 || addr < _regBaseAddr ||
        (uint32_t)addr + qty > (uint32_t)_regBaseAddr + _regCount) {
      buf[7] = fc | 0x80;
      buf[8] = 0x02;
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
    buf[8] = 0x01;
    pduLenResp = 2;
  }

  uint16_t mbapLength = pduLenResp + 1;
  buf[4] = mbapLength >> 8;
  buf[5] = mbapLength & 0xFF;
  client.write(buf, 7 + pduLenResp);
  return true;
}
