#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <Arduino.h>

// ---- Analog pins, one per channel ----
const uint8_t PRESSURE_PIN = A0; // static pressure transducer
const uint8_t DP_PIN       = A1; // differential pressure transducer

// ---- ADC / current-loop scaling ----
// All integers for efficiency (AVR chips have no FPU, so FP is emulated).
// Millivolts keep VREF exact if the board changes. Both loops use identical
// 250-ohm shunts, so one set of scaling constants covers both channels.
const uint16_t SHUNT_OHMS     = 250;  // ohms, current-sense shunt resistor
const uint16_t VREF_MV        = 5000; // mV, ADC reference voltage
const uint16_t ADC_MAX        = 1023; // 10-bit ADC
const uint16_t CURRENT_MIN_MA = 4;    // 0% output
const uint16_t CURRENT_MAX_MA = 20;   // 100% output

// ---- Network ----
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // >>> CUSTOMIZE: unique per device on your subnet
const uint16_t MODBUS_PORT = 502; // standard Modbus TCP port

// ---- Modbus register map ----
//   0x00 - static pressure transducer, current in centi-mA
//   0x01 - differential pressure transducer, current in centi-mA
const uint16_t HOLDING_REG_ADDR = 0x00;
const uint16_t NUM_HOLDING_REGS = 2;

#endif // CONSTANTS_H
