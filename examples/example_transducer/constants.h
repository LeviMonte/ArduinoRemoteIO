#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <Arduino.h>

// ---- Network ----
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // >>> CUSTOMIZE: unique per device on your subnet
const uint16_t MODBUS_PORT = 502; // Standard Modbus TCP port

// ---- Modbus register map ----
// 0x00 - static pressure transducer, current in centi-mA
// 0x01 - differential pressure transducer, current in centi-mA
const uint16_t HOLDING_REG_ADDR = 0x00;   // Modbus address of holdingRegs[0]
const uint16_t NUM_HOLDING_REGS = 2;      // >>> CUSTOMIZE: number of channels
                                           // (the .ino derives its buffer sizing
                                           // directly from this value now, so
                                           // there's nothing else to keep in sync)

// ---- Analog pins, one per channel ----
const uint8_t PRESSURE_PIN = A0; // Static pressure transducer
const uint8_t DP_PIN       = A1; // Differential pressure transducer

// ---- ADC / current-loop scaling ----
// All integers for efficiency since AVR chips lack an FPU (FP is emulated).
// Millivolts used to keep VREF exact if the board changes.
// Both loops use identical 250-ohm shunts, so one set of scaling
// constants covers both channels.
const uint16_t VREF_MV        = 5000; // mV, ADC reference voltage
const uint16_t ADC_MAX        = 1023; // 10-bit ADC
const uint16_t SHUNT_OHMS     = 250;  // ohms, current-sense shunt resistor
const uint16_t CURRENT_MIN_MA = 4;    // 0% output
const uint16_t CURRENT_MAX_MA = 20;   // 100% output

#endif
