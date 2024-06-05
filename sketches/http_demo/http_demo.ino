#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "Main House";
const char* password = "mainwifi";

// URL to access
const char* url = "https://esp32test.netlify.app/";
int cnt = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  
}

void loop() {
  cnt++;
  if(cnt==5){
    // Connect to WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Make an HTTP GET request
    if ((WiFi.status() == WL_CONNECTED)) { // Check if the device is connected to WiFi
      HTTPClient http;

      http.begin(url); // Specify the URL
      int httpCode = http.GET(); // Make the request

      // Check the returning code
      if (httpCode > 0) {
        // Get the request response payload
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      } else {
        Serial.println("Error on HTTP request");
      }

      http.end(); // Free the resources
      cnt = 0;
    }
  } else {
    Serial.print("fetching in ");
    Serial.print(5-cnt);
    Serial.println(" s");
  }
  delay(1000);
}
