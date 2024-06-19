#include "keypair/keypair.h"
#include "keypair/keypair.cpp"
#include "transaction/transaction.h"
#include "transaction/transaction.cpp"
#include <stdexcept>
#include "esp_system.h"
#include <string>
#include <WiFi.h>
#include <time.h>

// Replace with your network credentials
const char* ssid = "Main House";
const char* password = "mainwifi";

// NTP server details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

int cnt = 0;
Keypair keypair;

void printLocalTime() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}


void setup() {
    // Initialize serial communication
    Serial.begin(9600);  
    keypair.initialize(true);
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize and configure time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    
}

void loop() {
    cnt++;
    if(cnt == 5){ 
        // Print local time
        printLocalTime();
        try {
            Transaction transaction("trr4djiaaaaaaakakveacai", "call", "get_data", "null");
            std::vector<uint8_t> encoded = transaction.encode();
            for (auto byte : encoded) {
                Serial.printf("%02x", byte);
            }
            Serial.println();
        } catch (const std::exception& e) {
            Serial.printf("Exception: %s\n", e.what());
        }
        
        cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(5 - cnt);
    }
    delay(1000);
}

//1718809731
//1718809772
