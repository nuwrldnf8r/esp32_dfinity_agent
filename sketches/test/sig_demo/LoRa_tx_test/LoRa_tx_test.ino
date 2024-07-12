#include <LoRa.h>
#include <SPI.h>

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 26

unsigned long lastSendTime = 0; // Time of last message send
const unsigned long interval = 3000; // Interval between messages (3 seconds)

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
    sendMessage("Hello, world!");
    delay(3000);
}

void sendMessage(String message) {
    Serial.print("Sending message: ");
    Serial.println(message);

    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
}
