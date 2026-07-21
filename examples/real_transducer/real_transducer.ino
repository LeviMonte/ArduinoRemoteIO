/*
  real_transducer - the from-scratch reference behind ModbusResponder.

  This sketch is the ORIGINAL, hand-rolled version of the JLab SRGS
  cryogenics reader. It does NOT use the ModbusResponder library: instead
  it handles the Modbus TCP framing and Read Holding Registers (FC03) reply
  inline, in serveModbusRequest() below. The library was distilled from
  exactly this code, so it's kept here as a reference for anyone who wants
  to see what ModbusResponder::serve() does internally, or who needs to
  customize the protocol handling beyond what the library exposes.

  For everyday use, prefer the `example_transducer` sketch, which does the
  same job using the library. See `BasicHoldingRegisters` for the minimal
  API introduction.

  Function: reads two 4-20mA current-loop signals from 24V transducers
  (channel 0 = static pressure, channel 1 = differential pressure). Each
  loop drops across its own 250-ohm shunt; the ADC reads the shunt voltage
  and the result is served as current in centi-mA (hundredths of a mA).

  All math is integer-only: AVR chips have no FPU, so floating point is
  emulated and slow.

  The inline responder replaced the ArduinoModbus library, which cost
  ~700 bytes of static RAM (of 2048), ~300 bytes of runtime heap, and
  ~11 KB of flash for this single-function-code use case.
*/

#include <SPI.h>
#include <Ethernet.h>
#include "constants.h"

#define DEBUG_SERIAL 0   // set to 1 to print readings (also pulls in Serial, ~150 B RAM)
#define USE_STATIC_IP 1  // set to 0 for DHCP (DHCP uses ~150 B more RAM & ~3 KB flash)

#if USE_STATIC_IP
IPAddress staticIP(192, 168, 0, 177); // match the PLC subnet
#endif

EthernetServer ethServer(MODBUS_PORT);

// Holding registers served to the PLC (see the register map in constants.h).
uint16_t holdingRegs[NUM_HOLDING_REGS] = { 0 };

const uint8_t NUM_CHANNELS = 2;
const uint8_t ANALOG_PINS[NUM_CHANNELS] = { PRESSURE_PIN, DP_PIN };

// Moving-average filter to smooth ADC noise, one ring buffer per channel.
const uint8_t NUM_SAMPLES = 8;
uint16_t samples[NUM_CHANNELS][NUM_SAMPLES];
uint16_t sampleSum[NUM_CHANNELS];   // running totals, so we never re-sum the arrays
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

  Ethernet.init(10); // CS pin for most W5x00 shields
#if USE_STATIC_IP
  Ethernet.begin(mac, staticIP);
#else
  Ethernet.begin(mac);
#endif
  ethServer.begin();

#if DEBUG_SERIAL
  Serial.print(F("Modbus TCP server running at "));
  Serial.println(Ethernet.localIP());
#endif
}

// Updates the running sum for one channel and returns the filtered raw reading.
uint16_t readFilteredRaw(uint8_t ch) {
  uint16_t raw = analogRead(ANALOG_PINS[ch]);
  sampleSum[ch] -= samples[ch][sampleIndex[ch]]; // drop the sample being overwritten
  samples[ch][sampleIndex[ch]] = raw;
  sampleSum[ch] += raw;
  if (++sampleIndex[ch] >= NUM_SAMPLES) sampleIndex[ch] = 0;
  if (sampleCount[ch] < NUM_SAMPLES) sampleCount[ch]++;
  return sampleSum[ch] / sampleCount[ch]; // count >= 1 after the first call
}

void updateHoldingRegisters() {
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    uint16_t avgRaw = readFilteredRaw(ch);
    // current(mA) * 100 = avgRaw * VREF_MV * 100 / (ADC_MAX * SHUNT_OHMS)
    uint32_t centiMA = ((uint32_t)avgRaw * VREF_MV * 100UL) / ((uint32_t)ADC_MAX * SHUNT_OHMS);
    holdingRegs[ch] = constrain((uint16_t)centiMA, CURRENT_MIN_MA * 100, CURRENT_MAX_MA * 100);
#if DEBUG_SERIAL
    uint16_t whole = holdingRegs[ch] / 100;
    uint16_t frac  = holdingRegs[ch] % 100;
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
  Minimal Modbus TCP responder, handled inline.
  This is the exact logic the ModbusResponder library packages into
  serve(). Reads one request, replies to FC03, returns false to drop
  the connection on any framing error.
*/
bool serveModbusRequest(EthernetClient& client) {
  uint8_t buf[9 + 2 * NUM_HOLDING_REGS];

  // --- Read the 7-byte MBAP header ---
  if (client.readBytes(buf, 7) != 7) return false;

  uint16_t protoId = ((uint16_t)buf[2] << 8) | buf[3];
  uint16_t length  = ((uint16_t)buf[4] << 8) | buf[5];
  if (protoId != 0 || length < 2 || length > 255) return false;

  // --- Read the PDU (length already counts the 1-byte unit ID we read) ---
  uint8_t pduLen = length - 1;
  const uint8_t maxPduCapacity = sizeof(buf) - 7; // tie the cap to the real buffer size
  uint8_t toRead = pduLen > maxPduCapacity ? maxPduCapacity : pduLen;
  if (client.readBytes(buf + 7, toRead) != toRead) return false;
  for (uint8_t i = toRead; i < pduLen; i++) {
    if (client.read() < 0) return false; // drain any excess so the stream stays in sync
  }

  uint8_t fc = buf[7];
  uint8_t pduLenResp; // function code + payload, NOT including the unit ID

  if (fc == 0x03 && pduLen == 5) {
    uint16_t addr = ((uint16_t)buf[8] << 8) | buf[9];
    uint16_t qty  = ((uint16_t)buf[10] << 8) | buf[11];

    if (qty == 0 || addr < HOLDING_REG_ADDR ||
        (uint32_t)addr + qty > (uint32_t)HOLDING_REG_ADDR + NUM_HOLDING_REGS) {
      buf[7] = fc | 0x80;
      buf[8] = 0x02; // exception: illegal data address
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
    buf[8] = 0x01; // exception: illegal function
    pduLenResp = 2;
  }

  // --- Patch the MBAP length and send the reply ---
  uint16_t mbapLength = pduLenResp + 1; // +1 for the unit ID byte
  buf[4] = mbapLength >> 8;
  buf[5] = mbapLength & 0xFF;
  client.write(buf, 7 + pduLenResp); // full MBAP header (7) + PDU
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
      updateHoldingRegisters(); // keep registers fresh while the PLC polls
      if (client.available()) {
        if (!serveModbusRequest(client)) {
          client.stop();
          break;
        }
      }
    }
#if DEBUG_SERIAL
    Serial.println(F("PLC disconnected"));
#endif
  }

  delay(50);
}
