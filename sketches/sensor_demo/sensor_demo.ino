#include <DHT.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <LoRa.h>
#include <SPI.h>
#include "keypair.h"

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 26


#define GPS_TX 17 
#define GPS_RX 16
#define DHT_PIN 4
#define DHT_TYPE DHT22  

DHT dht(DHT_PIN, DHT_TYPE);
HardwareSerial GPS_SERIAL(2);
const int lightSensorPin = 36;

TinyGPSPlus gps;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(9600);
    while (!Serial);

    if (!LoRa.begin(433E6)) { // Set frequency to 433 MHz
        Serial.println("LoRa initialization failed!");
        while (1);
    }
    Serial.println("LoRa initialization succeeded!");

    Serial.println("Initializing LoRa module...");
    LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);

    GPS_SERIAL.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
    dht.begin();
  //pinMode(DHT_PIN, INPUT);
}

void loop() {
  float flat, flon;
  unsigned long age;
  int sensorValue = analogRead(lightSensorPin);
  Serial.print("Light intensity: ");
  Serial.println(sensorValue);
  
  
  float humidity = dht.readHumidity(); // Read humidity
  float temperature = dht.readTemperature(); // Read temperature in Celsius
  float hic = dht.computeHeatIndex(temperature, humidity, false);
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  
  
  //int value = digitalRead(DHT_PIN);
  //Serial.print("temp sensor: ");
  //Serial.println(value);

  readGPSData();
  
  delay(5000);
}

void readGPSData() {
  unsigned long start = millis(); // Get the current time
  while (millis() - start < 1000) { // Timeout after 1000 milliseconds (adjust as needed)
    if (GPS_SERIAL.available() > 0) {
      if (gps.encode(GPS_SERIAL.read())) {
        // If new GPS data is available, print it
        if (gps.location.isValid()) {
          Serial.print("Latitude: ");
          Serial.println(gps.location.lat(), 6);
          Serial.print("Longitude: ");
          Serial.println(gps.location.lng(), 6);
          if (gps.altitude.isValid()) {
          // Check if altitude data is valid
            Serial.print("Altitude: ");
            Serial.print(gps.altitude.meters());
            Serial.println(" meters");
          }
          return; // Exit the function after reading GPS data
        }
      }
    }
  }
  Serial.println("GPS data not available within timeout");
}

