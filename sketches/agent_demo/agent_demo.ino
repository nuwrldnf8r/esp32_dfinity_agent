#include "http_agent.h"
#include "keypair.h"
#include <stdexcept>
#include "esp_system.h"
#include <string>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <tinycbor/cbor.h>


// Replace with your network credentials
const char* ssid = "Main House";
const char* password = "mainwifi";

// NTP server details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

int cnt = 0;
Keypair keypair;


const std::string canisterId = "trr4d-jiaaa-aaaak-akvea-cai";

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    //keypair.initialize(true);

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
    if (cnt == 5) {
        // Print local time
        printLocalTime();
        try {
            Serial.println();
            if (WiFi.status() == WL_CONNECTED) {
                Serial.println("WiFi connected");

                Keypair kp = Keypair();
                kp.initialize();
                HttpAgent agent(canisterId, kp);
                Parameter p = Parameter((std::string)"yay this works");
                std::vector<Parameter> args = {p};
                std::vector<Parameter> result = agent.update("set_data", args);
                printf("**********************************");
                printf("Result: ");
                if(result.size() > 0) {
                    printf(result[0].parseText().c_str());
                    printf("\n");
                } 
               
                printf("**********************************");

                delay(5000);
                
                HttpAgent agent2(canisterId, kp);
                args = {};
                result = agent.query("get_data", args);
                printf("**********************************");
                printf("Result: ");
                printf(result[0].parseText().c_str());
                printf("\n");
                printf("**********************************");
                
            }
        } catch (const std::exception& e) {
            Serial.println("An exception occurred");
            Serial.printf("Exception: %s\n", e.what());
        }

        cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(5 - cnt);
    }
    delay(1000);
}
