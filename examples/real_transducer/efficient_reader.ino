/*
Pressure Reader - JLab SRGS Cryogenics Project

Reads two 4-20mA current loop signals from 24V transducers:
  channel 0 - static pressure transducer
  channel 1 - differential pressure transducer
Each loop drops across its own 250-ohm shunt; the ADC reads the
shunt voltage and the result is served as current in centi-mA.

Reads over Modbus TCP. ArduinoModbus library was replaced with
a low level response manager, only needing Read Holding Registers
on two consecutive registers. This change was made because libmodbus
was costing ~700 bytes (out of 2048) of static RAM plus runtime heap allocations
(~300 additional bytes) and ~11 KB of flash.

Levi Monte
7/20/2026
*/

#include <SPI.h>
#include <Ethernet.h>
#include "constants.h"

// Set to 0 to strip debug prints AND Serial itself (saves ~150 B RAM)
#define DEBUG_SERIAL 0

// Set to 1 to use a static IP instead of DHCP (saves ~150 B RAM & 3 KB flash)
#define USE_STATIC_IP 1
#if USE_STATIC_IP
IPAddress staticIP(192, 168, 0, 177); // match the PLC subnet
#endif

EthernetServer ethServer(MODBUS_PORT);

// Holding registers served to the PLC (see register map in constants.h)
uint16_t holdingRegs[NUM_HOLDING_REGS] = { 0 };

const uint8_t NUM_CHANNELS = 2;
const uint8_t ANALOG_PINS[NUM_CHANNELS] = { PRESSURE_PIN, DP_PIN };

// Average filter to smooth noise, one buffer per channel
// Only using integers instead of floats for efficiency

const uint8_t NUM_SAMPLES = 8;
uint16_t samples[NUM_CHANNELS][NUM_SAMPLES];
uint16_t sampleSum[NUM_CHANNELS];   // running totals to avoid re-summing the arrays
uint8_t  sampleIndex[NUM_CHANNELS];
uint8_t  sampleCount[NUM_CHANNELS]; // grows to NUM_SAMPLES during warm-up, then sticks

void setup() {
#if DEBUG_SERIAL
  Serial.begin(9600);
#endif
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    for (uint8_t i = 0; i < NUM_SAMPLES; i++) samples[ch][i] = 0;
    sampleSum[ch]   = 0;
    sampleIndex[ch] = 0;
    sampleCount[ch] = 0;
  }

  Ethernet.init(10);
#if USE_STATIC_IP
  Ethernet.begin(mac, staticIP); // Static IP
#else
  Ethernet.begin(mac); // DHCP
#endif

  ethServer.begin();

#if DEBUG_SERIAL
  Serial.print(F("Modbus TCP server running at "));
  Serial.println(Ethernet.localIP());
#endif
}

// Updates the running sum for one channel and returns the filtered raw reading
uint16_t readFilteredRaw(uint8_t ch) {
  uint16_t raw = analogRead(ANALOG_PINS[ch]);

  sampleSum[ch] -= samples[ch][sampleIndex[ch]]; // drop the sample being overwritten
  samples[ch][sampleIndex[ch]] = raw;
  sampleSum[ch] += raw;

  if (++sampleIndex[ch] >= NUM_SAMPLES) sampleIndex[ch] = 0;
  if (sampleCount[ch] < NUM_SAMPLES) sampleCount[ch]++;

  return sampleSum[ch] / sampleCount[ch]; // count >= 1 after first call
}

void updateHoldingRegisters() {
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    uint16_t avgRaw = readFilteredRaw(ch); // 10 bits

    // current_mA * 100 = avgRaw * VREF_MV * 100 / (ADC_MAX * SHUNT_OHMS)
    uint32_t centiMA = ((uint32_t)avgRaw * VREF_MV * 100UL) / ((uint32_t)ADC_MAX * SHUNT_OHMS);

    holdingRegs[ch] = constrain((uint16_t)centiMA, CURRENT_MIN_MA * 100, CURRENT_MAX_MA * 100);

#if DEBUG_SERIAL
    uint16_t whole = holdingRegs[ch] / 100;
    uint16_t frac = holdingRegs[ch] % 100;
    Serial.print(ch == 0 ? F("Pressure: ") : F("Diff pressure: "));
    Serial.print(whole);
    Serial.print(F("."));
    if (frac < 10) Serial.print('0');
    Serial.print(frac);
    Serial.print(F(" mA -> holding register "));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.println(holdingRegs[ch]);
#endif
  }
}

/*
Minimal Modbus TCP responder.

Request frame (MBAP header + PDU):
  [0-1] transaction ID   (echoed back)
  [2-3] protocol ID      (0)
  [4-5] length           (bytes remaining after this field, including unit ID)
  [6]   unit ID          (echoed back)
  [7]   function code
  [8-9] start address    (FC03)
  [10-11] register count (FC03)

Returns false if the connection should be dropped.
*/
bool serveModbusRequest(EthernetClient &client) {
  uint8_t buf[9 + 2 * NUM_HOLDING_REGS];

  if (client.readBytes(buf, 7) != 7) return false;

  uint16_t protoId = ((uint16_t)buf[2] << 8) | buf[3];
  uint16_t length  = ((uint16_t)buf[4] << 8) | buf[5];
  if (protoId != 0 || length < 2 || length > 255) return false;

  uint8_t pduLen = length - 1;
  const uint8_t maxPduCapacity = sizeof(buf) - 7; // tie the cap to real buffer size, not a magic number
  uint8_t toRead = pduLen > maxPduCapacity ? maxPduCapacity : pduLen;
  if (client.readBytes(buf + 7, toRead) != toRead) return false;
  for (uint8_t i = toRead; i < pduLen; i++) {
    if (client.read() < 0) return false;
  }

  uint8_t fc = buf[7];
  uint8_t pduLenResp; // FC + payload, NOT including unit ID

  if (fc == 0x03 && pduLen == 5) {
    uint16_t addr = ((uint16_t)buf[8] << 8) | buf[9];
    uint16_t qty  = ((uint16_t)buf[10] << 8) | buf[11];

    if (qty == 0 || addr < HOLDING_REG_ADDR ||
        (uint32_t)addr + qty > HOLDING_REG_ADDR + NUM_HOLDING_REGS) {
      buf[7] = fc | 0x80;
      buf[8] = 0x02;
      pduLenResp = 2;
    } else {
      buf[8] = qty * 2;
      uint8_t p = 9;
      for (uint16_t i = 0; i < qty; i++) {
        uint16_t v = holdingRegs[addr - HOLDING_REG_ADDR + i];
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

  uint16_t mbapLength = pduLenResp + 1; // +1 for the unit ID byte
  buf[4] = mbapLength >> 8;
  buf[5] = mbapLength & 0xFF;
  client.write(buf, 7 + pduLenResp); // full MBAP(7) + PDU
  return true;
}

void loop() {
  updateHoldingRegisters();

  EthernetClient client = ethServer.available();
  if (client) {
#if DEBUG_SERIAL
    Serial.println(F("PLC connected"));
#endif
    while (client.connected()) {
      updateHoldingRegisters(); // keeps registers updated w/fresh readings
      if (client.available()) {
        if (!serveModbusRequest(client)) {
          client.stop();
          break;
          delay(0); // Serial spam control
        }
      }
    }
#if DEBUG_SERIAL
    Serial.println(F("PLC disconnected"));
#endif
  }

  delay(50); // Less Serial spam
}
