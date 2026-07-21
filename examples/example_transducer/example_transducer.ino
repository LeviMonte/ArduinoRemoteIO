/*
  Pressure Reader - example for ArduinoRemoteIO
  based on JLab SRGS Cryogenics Project

  Reads 4-20mA current loop signals from 24V transducers. Each loop
  drops across its own 250-ohm shunt; the ADC reads the shunt voltage
  and the result is served as current in centi-mA over Modbus TCP
  (Read Holding Registers).
*/
#include <SPI.h>
#include <Ethernet.h>
#include "constants.h"

// MODBUS_MAX_REGS must be defined *before* including ModbusResponder.h.
// Deriving it directly from NUM_HOLDING_REGS means there's only one
// number to maintain in this whole sketch - the buffer size and the
// register count can no longer drift apart.
#define MODBUS_MAX_REGS NUM_HOLDING_REGS
#include <ModbusResponder.h>

#define DEBUG_SERIAL 0
#define USE_STATIC_IP 1

#if USE_STATIC_IP
IPAddress staticIP(192, 168, 0, 177); // >>> CUSTOMIZE: match your PLC's subnet
#endif

EthernetServer ethServer(MODBUS_PORT);

uint16_t holdingRegs[NUM_HOLDING_REGS] = { 0 };
ModbusResponder responder(holdingRegs, NUM_HOLDING_REGS, HOLDING_REG_ADDR);

// ============================================================
// >>> CUSTOMIZE HERE for 1, 2, or N channels <
// Set NUM_CHANNELS and list one analog pin per channel in
// ANALOG_PINS. Everything below (filtering, register updates)
// already loops over NUM_CHANNELS, so this is the only place
// you need to touch to scale up or down.
// (Also update NUM_HOLDING_REGS in constants.h to match - that's
// the only other number tied to channel count.)
// ============================================================
const uint8_t NUM_CHANNELS = 2;
const uint8_t ANALOG_PINS[NUM_CHANNELS] = { PRESSURE_PIN, DP_PIN };
// ============================================================

const uint8_t NUM_SAMPLES = 8;
uint16_t samples[NUM_CHANNELS][NUM_SAMPLES];
uint16_t sampleSum[NUM_CHANNELS];
uint8_t  sampleIndex[NUM_CHANNELS];
uint8_t  sampleCount[NUM_CHANNELS];

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

// Updates the running sum for one channel and returns the filtered raw reading
uint16_t readFilteredRaw(uint8_t ch) {
  uint16_t raw = analogRead(ANALOG_PINS[ch]);
  sampleSum[ch] -= samples[ch][sampleIndex[ch]];
  samples[ch][sampleIndex[ch]] = raw;
  sampleSum[ch] += raw;
  if (++sampleIndex[ch] >= NUM_SAMPLES) sampleIndex[ch] = 0;
  if (sampleCount[ch] < NUM_SAMPLES) sampleCount[ch]++;
  return sampleSum[ch] / sampleCount[ch];
}

void updateHoldingRegisters() {
  for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
    uint16_t avgRaw = readFilteredRaw(ch);
    // ============================================================
    // >>> CUSTOMIZE HERE if your scaling math differs per channel <
    // This assumes all channels share the same VREF/shunt/range.
    // If channels need different scaling, swap these constants for
    // per-channel arrays the same way ANALOG_PINS is one.
    // ============================================================
    uint32_t centiMA = ((uint32_t)avgRaw * VREF_MV * 100UL) / ((uint32_t)ADC_MAX * SHUNT_OHMS);
    holdingRegs[ch] = constrain((uint16_t)centiMA, CURRENT_MIN_MA * 100, CURRENT_MAX_MA * 100);
#if DEBUG_SERIAL
    uint16_t whole = holdingRegs[ch] / 100;
    uint16_t frac = holdingRegs[ch] % 100;
    Serial.print(F("Channel "));
    Serial.print(ch);
    Serial.print(F(": "));
    Serial.print(whole);
    Serial.print(F("."));
    if (frac < 10) Serial.print('0');
    Serial.print(frac);
    Serial.println(F(" mA"));
#endif
  }
}

void loop() {
  updateHoldingRegisters();
  EthernetClient client = ethServer.available();
  if (client) {
#if DEBUG_SERIAL
    Serial.println(F("Client connected"));
#endif
    while (client.connected()) {
      updateHoldingRegisters();
      if (client.available()) {
        if (!responder.serve(client)) {
          client.stop();
          break;
        }
      }
    }
#if DEBUG_SERIAL
    Serial.println(F("Client disconnected"));
#endif
  }
  delay(50);
}
