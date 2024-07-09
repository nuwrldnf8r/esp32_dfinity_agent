
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
  0x04 - Request Owner Principal
  0x05 - Request Data
     requestID 32 bytes, data 0-255 bytes
  0x06 - Cancel Request
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
#define RFM95_CS 5
#define RFM95_RST 14
#define RFM95_INT 26

#define MAGIC_PREFIX "ESMSG"
#define PACKET_PREFIX "ESP"

#define STATUS_INITIALIZING 0x00
#define STATUS_INITIALIZED 0x01
#define STATUS_INITIALIZE_WITH_WIFI 0x02
#define STATUS_CONNECTED 0x03
#define STATUS_CHECKING 0x04
#define STATUS_NOT_REGISTERED 0x05
#define STATUS_REGISTERING 0x06
#define STATUS_REGISTERED 0x07
#define STATUS_GOT_PRINCIPAL 0x08
#define STATUS_ERROR_NO_PRINCIPAL 0xF5

#define STATUS_ERROR_WIFI_NOTCONNECTED 0xF1
#define STATUS_ERROR_PRINCIPAL_MISMATCH 0xF2
#define STATUS_ERROR_FETCH_LOCAL_TIME 0xF3

#define MESSAGE_TYPE_STATUS 0x00
#define MESSAGE_TYPE_DATA 0x01
#define MESSAGE_TYPE_LOGGING 0x02
#define MESSAGE_TYPE_OWNER_PRINCIPAL 0x04
#define MESSAGE_TYPE_ERROR 0xFF

#define REQUEST_TYPE_READ_STATUS 0x00
#define REQUEST_TYPE_WIFI_CREDENTIALS 0x01
#define REQUEST_TYPE_OWNER_PRINCIPAL 0x02
#define REQUEST_TYPE_REQUEST_PRINCIPAL 0x03
#define REQUEST_TYPE_REQUEST_OWNER_PRINCIPAL 0x04
#define REQUEST_TYPE_REQUEST_DATA 0x05
#define REQUEST_TYPE_CANCEL_REQUEST 0x06
#define REQUEST_TYPE_ERROR 0xFF


const std::string canisterId = "ddya3-4iaaa-aaaak-akwka-cai";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

bool timeInitialized = false;
bool deviceConnected = false;
uint8_t status_;
int value = 0;
std::string principal_ = "";
bool invalidPrincipal_ = false;
bool isRegistered_ = false;

void log(const std::string& message){
  std::vector<uint8_t> _m = {MESSAGE_TYPE_LOGGING};
  std::string _message = MAGIC_PREFIX + Utils::bytes_to_hex(_m) + message + '\n';
  Serial.print(_message.c_str());
}

bool isRegistered(){
  if(isRegistered_) return true;
  //check on the network
  Serial.println("Checking if registered");
  try{
    HttpAgent agent(canisterId, keypair);
    std::vector<Parameter> args = {};
    std::vector<Parameter> result = agent.query("is_registered", args);
    if(result.size() > 0){
      isRegistered_ = result[0].parseBool()==true;
      Serial.println("Is registered: " + String(isRegistered_));
      return isRegistered_;
    } else {
      return false;
    }
  } catch(const std::exception& e){
    Serial.printf("Exception: %s\n", e.what());
    return false;
  } 
  //return false;
}

void registerOnNetwork(){
  Serial.println("Registering on network");
  try{
    HttpAgent agent(canisterId, keypair);
    std::vector<Parameter> args = {Parameter(principal_)};
    std::vector<Parameter> result = agent.update("register", args);
    Serial.println("Registered on network");
    status_ = STATUS_REGISTERED;
  } catch(const std::exception& e){
    Serial.printf("Exception: %s\n", e.what());
  }

}

void processWifiCredentials(const String& wifi_credentials){
    if(invalidPrincipal_){
        status_ = STATUS_ERROR_PRINCIPAL_MISMATCH;
        return;
    }
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

void processOwnerPrincipal(const String& owner_principal){
    log("Processing owner principal");
    String _principal = owner_principal.substring(strlen(MAGIC_PREFIX) + 2);
    //check if principal exists 
    if(principal_.length()>0){
        Serial.println("Principal exists");
        Serial.println(principal_.c_str());
        //if exists, check if == stored principal
        if(principal_ != _principal.c_str()){
            //if not, error
            status_ = STATUS_ERROR_PRINCIPAL_MISMATCH;
            invalidPrincipal_ = true;
            return;
        }
    } else {
        //if not - store
        Serial.println("Storing principal:");
        Serial.println(_principal.c_str());
        principal_ = _principal.c_str();
        writeVector("principal", Utils::string_to_vector(principal_));
    }

    //if not STATUS_ERROR_PRINCIPAL_MISMATCH, register on network
    if(!status_ == STATUS_ERROR_PRINCIPAL_MISMATCH){
        //register on network
        registerOnNetwork();
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

std::string getRequest(const uint8_t& requestType, const std::string& data){
  return MAGIC_PREFIX + Utils::bytes_to_hex({requestType}) + data;
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

void uploadData(const std::string& data){
  Serial.println("Uploading data");
  try{
    HttpAgent agent(canisterId, keypair);
    std::vector<Parameter> args = {Parameter(data)};
    std::vector<Parameter> result = agent.update("upload_data", args);
    Serial.println("Data uploaded");
  } catch(const std::exception& e){
    Serial.printf("Exception: %s\n", e.what());
  }
}

void setup(){
  Serial.begin(9600); //115200
  while (!Serial);

  preferences.begin("_", false);
  //preferences.clear();
  //preferences.end();

  Serial.println("Initializing LoRa module...");
  LoRa.setPins(RFM95_CS, RFM95_RST, RFM95_INT);
  
  
  
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

  //get principal from storage
  std::vector<uint8_t> _principal = readVector("principal");
  if(_principal.size() > 0){
    principal_ = Utils::vector_to_string(_principal);
    Serial.print("Principal: ");
    Serial.println(principal_.c_str());
    //check if registered on the network
  } else {
    Serial.println("No principal found");
  }
  
  //get wifi credientials from storage
  std::vector<uint8_t> ssid_ = readVector("ssid");
  std::vector<uint8_t> password_ = readVector("password");
  if(ssid_.size() > 0 && password_.size() > 0){
    setWIFI(Utils::vector_to_string(ssid_).c_str(), Utils::vector_to_string(password_).c_str());
    if(WiFi.status() == WL_CONNECTED){
      status_ = STATUS_CONNECTED;    
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
      printLocalTime(); 
      if(!isRegistered() && principal_.length()>0){
        registerOnNetwork();
      } else {
        status_ = STATUS_REGISTERED;
      }
    } else {
      status_ = STATUS_ERROR_WIFI_NOTCONNECTED;
    }
  }

  if (!LoRa.begin(433E6)) { // Set frequency to 433 MHz
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void loop(){
  /*
  if(!timeInitialized && WiFi.status() == WL_CONNECTED){
    struct tm timeinfo;
    Serial.println("Fetching time");
    if (!getLocalTime(&timeinfo)) {
        status_ = STATUS_ERROR_FETCH_LOCAL_TIME;
        Serial.println("Failed to get time");
    } else {
      Serial.println("Got time");
      timeInitialized = true;
    }
  }
  */
 
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
        case REQUEST_TYPE_OWNER_PRINCIPAL:
          if(principal_.length() == 0){
            status_ = STATUS_GOT_PRINCIPAL;
            Serial.print(getStatus().c_str());
            processOwnerPrincipal(incomingData);
          }
          
          break;
        case REQUEST_TYPE_ERROR:
          Serial.print(getError("Invalid message").c_str());
          break;
        case REQUEST_TYPE_WIFI_CREDENTIALS:
          processWifiCredentials(incomingData);
          break;
        case REQUEST_TYPE_REQUEST_OWNER_PRINCIPAL:
          

          Serial.print(getRequest(MESSAGE_TYPE_OWNER_PRINCIPAL, principal_).c_str());
          break;
        default:
          Serial.print(getError("Unknown request").c_str());
          break;
      }
      //Serial.print(getStatus().c_str());

  }

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read packet
    std::vector<uint8_t> packet;
    while (LoRa.available() && packet.size() < packetSize) {
      packet.push_back(LoRa.read());
    }
    if(validatePacket(packet)){
      Serial.println("Valid packet");
      std::vector<uint8_t> _packet = {packet.begin() + strlen(PACKET_PREFIX), packet.end()};
      std::string hex = Utils::bytes_to_hex(_packet);
      Serial.println(hex.c_str());
      uploadData(hex);
    } else {
      Serial.println("Invalid packet");
    }
  }
  
  
}

bool validatePacket(const std::vector<uint8_t>& packet){
  Serial.println("Validating packet:");
  
  
  if(packet.size() < strlen(PACKET_PREFIX) + 4 + 6){
    Serial.println("Invalid packet size");
    return false;
  }
  std::string prefix = Utils::vector_to_string({packet.begin(), packet.begin() + strlen(PACKET_PREFIX)});
  if(prefix != PACKET_PREFIX){
    Serial.println("Invalid prefix");
    return false;
  }
  std::vector<uint8_t> _packet = {packet.begin()+strlen(PACKET_PREFIX), packet.end()};
  //validate checksum
  std::vector<uint8_t> chk_1(_packet.begin(), _packet.begin() +4);
  std::vector<uint8_t> to_hash(_packet.begin() + 4, _packet.end());
  std::vector<uint8_t> chk_2 = Utils::sha256(to_hash);
  chk_2 = std::vector<uint8_t>(chk_2.begin(), chk_2.begin() + 4);
  if(chk_1 != chk_2){
    Serial.println("Invalid checksum");
    return false;
  }
  return true;
}