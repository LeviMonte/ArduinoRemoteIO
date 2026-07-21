#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

// ---- Network ----
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // >>> CUSTOMIZE: unique per device on your subnet
const uint16_t MODBUS_PORT = 502;

// ---- Modbus register map ----
const uint16_t HOLDING_REG_ADDR  = 0;   // Modbus address of holdingRegs[0]
const uint16_t NUM_HOLDING_REGS  = 2;   // >>> CUSTOMIZE: must match MODBUS_MAX_REGS in the .ino

// ---- Analog pins, one per channel ----
const uint8_t PRESSURE_PIN = A0;
const uint8_t DP_PIN       = A1;

// ---- ADC / current-loop scaling ----
const uint16_t VREF_MV     = 5000; // mV, ADC reference voltage
const uint16_t ADC_MAX     = 1023; // 10-bit ADC
const uint16_t SHUNT_OHMS  = 250;  // ohms, current-sense shunt resistor
const uint8_t  CURRENT_MIN_MA = 4;
const uint8_t  CURRENT_MAX_MA = 20;

#endif
