
#include "http_agent.h"
#include "keypair.h"
#include "utils.h"
//#include "eeprom_vector_storage.h"
#include <LoRa.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>

Keypair keypair;
Preferences preferences;

/*
  Outgoing message types:
  0x00 - Status
    status byte followed by status code
  0x01 - Data  
    data byte, message type, followed by data (for type 0x04 - data is cbor encoded)

  Incoming message types:
  0x00 - Read Status
  0x01 - Wifi Credentials
    ssid_length 1 byte, password length 1 byte, ssid, password
  0x02 - Set Owner Principal
  0x03 - Request Principal
  0x04 - Request Data
     requestID 32 bytes, data 0-255 bytes
  0x05 - Cancel Request
      requestID 32 bytes
  0xFF - error
    
  Status codes:
  Status 0x00 - initializing
  Status 0x01 - initialized with principal - ready to receive wifi credentials
  Status 0x02 - has wifi credentials - ready to connect
  Status 0x03 - connected to wifi and initialized time 
  Status 0x04 - checking if registered
  Status 0x05 - not registered - ready to receive owner Principal and register on network
  Status 0x06 - registering - waiting for confirmation
  Status 0x07 - registered - ready to receive data
*/

#define MAGIC_PREFIX "ESMSG"

#define STATUS_INITIALIZING 0x00
#define STATUS_INITIALIZED 0x01
#define STATUS_INITIALIZE_WITH_WIFI 0x02
#define STATUS_CONNECTED 0x03
#define STATUS_CHECKING 0x04
#define STATUS_NOT_REGISTERED 0x05
#define STATUS_REGISTERING 0x06
#define STATUS_REGISTERED 0x07
#define STATUS_ERROR_WIFI_NOTCONNECTED 0xF1

#define MESSAGE_TYPE_STATUS 0x00
#define MESSAGE_TYPE_DATA 0x01
#define MESSAGE_TYPE_LOGGING 0x02
#define MESSAGE_TYPE_ERROR 0xFF

#define REQUEST_TYPE_READ_STATUS 0x00
#define REQUEST_TYPE_WIFI_CREDENTIALS 0x01
#define REQUEST_TYPE_OWNER_PRINCIPAL 0x02
#define REQUEST_TYPE_REQUEST_PRINCIPAL 0x03
#define REQUEST_TYPE_REQUEST_DATA 0x04
#define REQUEST_TYPE_CANCEL_REQUEST 0x05
#define REQUEST_TYPE_ERROR 0xFF



bool deviceConnected = false;
uint8_t status_;
int value = 0;

void log(const std::string& message){
  std::vector<uint8_t> _m = {MESSAGE_TYPE_LOGGING};
  std::string _message = MAGIC_PREFIX + Utils::bytes_to_hex(_m) + message + '\n';
  Serial.print(_message.c_str());
}

void processWifiCredentials(const String& wifi_credentials){
    log("Processing wifi credentials");
    String _data = wifi_credentials.substring(strlen(MAGIC_PREFIX) + 2);
    if(_data.length() < 2){
        return;
    }
    int lssid = int(Utils::hex_to_bytes(_data.substring(0, 2).c_str())[0]);
    int lpassword = int(Utils::hex_to_bytes(_data.substring(2, 4).c_str())[0]);
    std::string ssid = _data.substring(4, 4 + lssid).c_str();
    std::string password = _data.substring(4 + lssid, 4 + lssid + lpassword).c_str();
    status_ = STATUS_INITIALIZE_WITH_WIFI;
    setWIFI(ssid.c_str(), password.c_str());
    
    if(WiFi.status() == WL_CONNECTED){
        //store wifi credentials
        writeVector("ssid", Utils::string_to_vector(ssid));
        writeVector("password", Utils::string_to_vector(password));
    } else {
        status_ = STATUS_ERROR_WIFI_NOTCONNECTED;
    }
    
}

void setWIFI(const char* ssid, const char* password){
    Serial.println("Setting up Wi-Fi");
    Serial.println("SSID: " + String(ssid));
    Serial.println("Password: " + String(password));
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    status_ = STATUS_CONNECTED;
    //TODO: save wifi settings to EEPROM
    //TODO: set time
}



std::string getStatus(){
  std::vector<uint8_t> _m = {MESSAGE_TYPE_STATUS, status_, '\n'};
  return MAGIC_PREFIX + Utils::bytes_to_hex(_m);
}

std::string getError(const std::string& error){
  return MAGIC_PREFIX + Utils::bytes_to_hex({MESSAGE_TYPE_ERROR}) + error;
}

uint8_t requestType(const String& data){
  if(data.length() < strlen(MAGIC_PREFIX) + 2){
    return REQUEST_TYPE_ERROR;
  }
  if(!data.startsWith(MAGIC_PREFIX)){
    return REQUEST_TYPE_ERROR;
  }
  String _data = data.substring(strlen(MAGIC_PREFIX));
  std::vector<uint8_t> _m = Utils::hex_to_bytes(_data.c_str());
  return _m[0];
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

void printVector(const std::vector<uint8_t>& vec, const String& name) {
    Serial.print(name + ": ");
    for (size_t i = 0; i < vec.size(); ++i) {
        Serial.print(vec[i], HEX);
        if (i < vec.size() - 1) {
            Serial.print(", ");
        }
    }
    Serial.println();
}


void setup(){
  Serial.begin(9600); //115200
  preferences.begin("_", false);
  //preferences.clear();
  //preferences.end();

  
   std::vector<uint8_t> private_key = readVector("pk");
  if(private_key.size() > 0){
    keypair.initialize(private_key);
  } else {
    Serial.println("Generating new keypair");
    keypair.initialize();
    //store private key
    writeVector("pk", keypair.getPrivateKey());
  }
  status_ = STATUS_INITIALIZED;
  
  //get wifi credientials from storage
  std::vector<uint8_t> ssid_ = readVector("ssid");
  std::vector<uint8_t> password_ = readVector("password");
  if(ssid_.size() > 0 && password_.size() > 0){
    setWIFI(Utils::vector_to_string(ssid_).c_str(), Utils::vector_to_string(password_).c_str());
    if(WiFi.status() == WL_CONNECTED){
      status_ = STATUS_CONNECTED;
    } else {
      status_ = STATUS_ERROR_WIFI_NOTCONNECTED;
    }
  }
  
}

void loop(){
  if (Serial.available() > 0) {
      // Read the incoming data
      String incomingData = Serial.readStringUntil('\n');
      
      // Print the received data
      Serial.println();
      //Serial.print("Received: ");
      //Serial.println(incomingData);
      switch(requestType(incomingData)){
        case REQUEST_TYPE_READ_STATUS:
          Serial.print(getStatus().c_str());
          break;
        case REQUEST_TYPE_ERROR:
          Serial.print(getError("Invalid message").c_str());
          break;
        case REQUEST_TYPE_WIFI_CREDENTIALS:
          processWifiCredentials(incomingData);
          break;
        default:
          Serial.print(getError("Unknown request").c_str());
          break;
      }
      //Serial.print(getStatus().c_str());

  }
  
  
}