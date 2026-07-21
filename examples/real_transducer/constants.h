// Setup pins

const uint8_t PRESSURE_PIN = A0; // Static pressure transducer
const uint8_t DP_PIN       = A1; // Differential pressure transducer

// Circuit constants, all integers for efficiency since AVR chip lacks FPU & is only emulated
// Millivolts used to keep VREF exact if we switched boards
// Both loops use identical 250-ohm shunts (could change this but I'm assuming they're the same), 
// so one set of scaling constants covers both

const uint16_t SHUNT_OHMS     = 250;   // Shunt resistor ohms (one per channel)
const uint16_t VREF_MV        = 5000;  // ADC reference voltage, millivolts
const uint16_t ADC_MAX        = 1023;  // 10-bit ADC

const uint16_t CURRENT_MIN_MA = 4;  // 0% output
const uint16_t CURRENT_MAX_MA = 20; // 100% output

// Network settings with PLC
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Arduino MAC address
const uint16_t MODBUS_PORT = 502; // Standard Modbus TCP port

// Register map:
//   0x00 - static pressure transducer, current in centi-mA
//   0x01 - differential pressure transducer, current in centi-mA
const uint16_t HOLDING_REG_ADDR = 0x00;
const uint16_t NUM_HOLDING_REGS = 2;
