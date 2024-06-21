#include "keypair/keypair.h"
#include "keypair/keypair.cpp"
#include "transaction/transaction.h"
#include "transaction/transaction.cpp"
#include <stdexcept>
#include "esp_system.h"
#include <string>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// Replace with your network credentials
const char* ssid = "Main House";
const char* password = "mainwifi";

// NTP server details
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

int cnt = 0;
Keypair keypair;

WiFiClientSecure client;

String urlEncode(const String &value) {
    String encoded;
    char hex[4];
    for (int i = 0; i < value.length(); i++) {
        char c = value.charAt(i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

const String canisterId = "trr4d-jiaaa-aaaak-akvea-cai";
const String baseURL = "https://ic0.app/api/v2/canister/";
const String encodedCanisterId = urlEncode(canisterId);
const String host = baseURL + encodedCanisterId + "/call";
const int httpsPort = 443;

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void sendReadStateRequest(const std::string& requestId) {
    try {
        Transaction transaction("trr4djiaaaaaaakakveacai", "call", "get_data", "null");
        std::vector<std::vector<std::string>> paths = {{"request_status", requestId, "reply"}};

        std::vector<uint8_t> readStateRequest = transaction.createReadStateRequest("trr4d-jiaaa-aaaak-akvea-cai", paths);
        for (auto byte : readStateRequest) {
            Serial.printf("%02x", byte);
        }
        Serial.println();

        String url = baseURL + urlEncode(canisterId) + "/read_state";

        if (WiFi.status() == WL_CONNECTED) {
            client.setInsecure();
            HTTPClient http;
            http.begin(client, url);
            http.addHeader("Content-Type", "application/cbor");
            Serial.print("Connecting to ");
            Serial.println(url);
            int httpResponseCode = http.POST(readStateRequest.data(), readStateRequest.size());
            if (httpResponseCode > 0) {
                Serial.print("Response code: ");
                Serial.println(httpResponseCode);
                Serial.print("Response: ");
                String response = http.getString();
                Serial.println(response);

                // Assuming you get a CBOR response and want to print it as hex
                std::vector<uint8_t> responseBytes(response.begin(), response.end());
                for (auto byte : responseBytes) {
                    Serial.printf("%02x", byte);
                }
                Serial.println();
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
                Serial.print("Error message: ");
                Serial.println(http.errorToString(httpResponseCode).c_str());
            }
            http.end();
        }
    } catch (const std::exception& e) {
        Serial.println("An exception occurred");
        Serial.printf("Exception: %s\n", e.what());
    }
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
            Transaction transaction("trr4djiaaaaaaakakveacai", "call", "get_data", "null");
            std::vector<uint8_t> encoded = transaction.encode();
            if (WiFi.status() == WL_CONNECTED) {
                client.setInsecure();
                HTTPClient http;
                String url = host;
                http.begin(client, url);
                http.addHeader("Content-Type", "application/cbor");
                Serial.print("Connecting to ");
                Serial.println(url);
                int httpResponseCode = http.POST(encoded.data(), encoded.size());
                if (httpResponseCode > 0) {
                    Serial.print("Response code: ");
                    Serial.println(httpResponseCode);
                    Serial.print("Response: ");
                    String response = http.getString();
                    Serial.println(response);

                    // Assuming you get a CBOR response and want to print it as hex
                    std::vector<uint8_t> responseBytes(response.begin(), response.end());
                    for (auto byte : responseBytes) {
                        Serial.printf("%02x", byte);
                    }
                    Serial.println();

                    std::string requestId = transaction.generateRequestId();
                    sendReadStateRequest(requestId);
                } else {
                    Serial.print("Error on sending POST: ");
                    Serial.println(httpResponseCode);
                    Serial.print("Error message: ");
                    Serial.println(http.errorToString(httpResponseCode).c_str());
                }
                http.end();
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
