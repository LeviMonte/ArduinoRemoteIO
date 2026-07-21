/*
  BasicHoldingRegisters - minimal example for the ModbusResponder library.

  Serves two holding registers over Modbus TCP (function code 0x03) to a
  PLC or any Modbus master. This example fills the registers with simple
  demo values so you can focus on the library's API; a real sketch would
  write sensor readings into holdingRegs[] instead.

  Wiring: an Arduino with a W5x00 Ethernet shield/module (uses the built-in
  Ethernet library). Adapt the transport to your board as needed - the
  responder works with any object implementing the Arduino Client interface.

  Register map (base address 0):
    holdingRegs[0] -> Modbus address 0
    holdingRegs[1] -> Modbus address 1
*/

#include <SPI.h>
#include <Ethernet.h>

// Size the responder's buffer BEFORE including the header (see library docs).
#define MODBUS_MAX_REGS 2
#include <ModbusResponder.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // unique per device on your subnet
IPAddress staticIP(192, 168, 0, 177);               // match your PLC's subnet
const uint16_t MODBUS_PORT = 502;                   // standard Modbus TCP port

EthernetServer ethServer(MODBUS_PORT);

uint16_t holdingRegs[MODBUS_MAX_REGS] = { 0 };
ModbusResponder responder(holdingRegs, MODBUS_MAX_REGS, /* baseAddr */ 0);

void updateRegisters() {
  // Replace this with your own data source (analogRead, sensor libraries, etc.).
  holdingRegs[0]++;               // a counter, just to show live values
  holdingRegs[1] = millis() / 1000; // seconds since boot
}

void setup() {
  Ethernet.init(10);              // CS pin for most W5x00 shields
  Ethernet.begin(mac, staticIP);
  ethServer.begin();
}

void loop() {
  updateRegisters();

  EthernetClient client = ethServer.available();
  if (client) {
    while (client.connected()) {
      updateRegisters();          // keep values fresh while the master is polling
      if (client.available()) {
        if (!responder.serve(client)) {
          client.stop();
          break;
        }
      }
    }
  }

  delay(50);
}
