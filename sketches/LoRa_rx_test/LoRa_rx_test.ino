#include <LoRa.h>
#include <SPI.h>

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 26

void setup() {
    Serial.begin(9600);
  while (!Serial);

  // Initialize LoRa module
  Serial.println("Initializing LoRa module...");
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);

  if (!LoRa.begin(433E6)) { // Set frequency to 433 MHz
    Serial.println("LoRa initialization failed!");
    while (1);
  }

  Serial.println("LoRa initialization succeeded!");

}

void loop() {
  // Try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read packet
    Serial.print("Received packet: ");
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    Serial.println();
  }
}