#include <DHT.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <LoRa.h>
#include <SPI.h>
#include <Preferences.h>
#include "keypair.h"
#include "utils.h"

#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 26

#define GPS_TX 17 
#define GPS_RX 16
#define DHT_PIN 4
#define DHT_TYPE DHT22  

//Fields:
#define FIELD_TEMP 0x01
#define FIELD_HUMIDITY 0x02
#define FIELD_LIGHT_INTENSITY 0x03
#define FIELD_LATITUDE 0x04
#define FIELD_LONGITUDE 0x05
#define FIELD_ALTITUDE 0x06
#define FIELD_PUBLIC_KEY 0x07
#define FIELD_SIGNATURE 0x08

#define PREFIX "ESP"

DHT dht(DHT_PIN, DHT_TYPE);
HardwareSerial GPS_SERIAL(2);
const int lightSensorPin = 36;

TinyGPSPlus gps;

Keypair keypair;
Preferences preferences;

std::string field(const uint8_t& name, const std::string& value) {
    //assuming no field's value is greater than 255 bytes
    std::string _val = value;
    if(value=="nan") _val = "";
    std::vector<uint8_t> bytes_value = Utils::string_to_vector({_val.begin(), _val.end()});
    uint8_t size = static_cast<uint8_t>(bytes_value.size());
    return Utils::bytes_to_hex({name}) + Utils::bytes_to_hex({size}) + Utils::bytes_to_hex(bytes_value);
}

std::string field(const uint8_t& name, std::vector<uint8_t>& value) {
    uint8_t size = static_cast<uint8_t>(value.size());
    return Utils::bytes_to_hex({name}) + Utils::bytes_to_hex({size}) + Utils::bytes_to_hex(value);
}

std::vector<uint8_t> createPacket() {
    
    //public key
    std::vector<uint8_t> public_key = keypair.publicKey();
    
    //sensor data
    float humidity = dht.readHumidity(); // Read humidity
    float temperature = dht.readTemperature(); // Read temperature in Celsius
    int lightIntensity = analogRead(lightSensorPin);

    unsigned long start = millis();
    float latitude, longitude, altitude;
    bool gpsDataAvailable = false;
    while (millis() - start < 2000) { // Timeout after 1000 milliseconds (adjust as needed)
        if (GPS_SERIAL.available() > 0) {
            if (gps.encode(GPS_SERIAL.read())) {
                // If new GPS data is available, print it
                if (gps.location.isValid()) {
                    latitude = gps.location.lat();
                    longitude = gps.location.lng();
                    gpsDataAvailable = true;
                }
                if (gps.altitude.isValid()){
                    altitude = gps.altitude.meters();
                }
                break;
            }
        }
    }

    std::string packet = field(FIELD_PUBLIC_KEY, public_key);
    packet += field(FIELD_TEMP, std::to_string(temperature));
    packet += field(FIELD_HUMIDITY, std::to_string(humidity));
    packet += field(FIELD_LIGHT_INTENSITY, std::to_string(lightIntensity));
    if(gpsDataAvailable){
        packet += field(FIELD_LATITUDE, std::to_string(latitude));
        packet += field(FIELD_LONGITUDE, std::to_string(longitude));
        packet += field(FIELD_ALTITUDE, std::to_string(altitude));
    } else {
        packet += field(FIELD_LATITUDE, "");
        packet += field(FIELD_LONGITUDE, "");
        packet += field(FIELD_ALTITUDE, "");
    }
    std::vector<uint8_t> signature = keypair.sign(Utils::sha256(Utils::hex_to_bytes(packet)));
    packet = field(FIELD_SIGNATURE, signature) + packet;

    //calculate checksum
    std::vector<uint8_t> chk = Utils::sha256(Utils::hex_to_bytes(packet));
    chk = std::vector<uint8_t>(chk.begin(), chk.begin() + 4);
    packet = Utils::bytes_to_hex(chk) + packet;
    std::vector<uint8_t> _prefix = Utils::string_to_vector(PREFIX);
    std::vector<uint8_t> _packet = Utils::hex_to_bytes(packet);
    _prefix.insert(_prefix.end(), _packet.begin(), _packet.end());
    return _prefix;
}

void sendPacket(const std::vector<uint8_t>& message) {
    Serial.print("Sending message: ");

    LoRa.beginPacket();
    for (uint8_t byte : message) {
        LoRa.write(byte);
    }
    LoRa.endPacket();
}

void writeVector(const char* key, const std::vector<uint8_t>& vec) {
    preferences.putBytes(key, vec.data(), vec.size());
}

std::vector<uint8_t> readVector(const char* key) {
    size_t size = preferences.getBytesLength(key);
    std::vector<uint8_t> vec(size);
    preferences.getBytes(key, vec.data(), size);
    return vec;
}

void setup() {
    // put your setup code here, to run once:

    Serial.begin(9600);
    while (!Serial);

    preferences.begin("_", false);

    std::vector<uint8_t> private_key = readVector("pk");
    if(private_key.size() > 0){
        keypair.initialize(private_key);
    } else {
        Serial.println("Generating new keypair");
        keypair.initialize();
        //store private key
        writeVector("pk", keypair.getPrivateKey());
    }

    Serial.println("Initializing LoRa module...");
    LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);

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
    Serial.println("creating packet");
    sendPacket(createPacket());
  
  delay(30000);
}









