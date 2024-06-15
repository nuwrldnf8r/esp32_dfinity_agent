#include "keypair/keypair.h"
#include "keypair/keypair.cpp"
#include <stdexcept>
#include "esp_system.h"

int cnt = 0;
Keypair keypair = Keypair();

void setup() {
  // Initialize serial communication
  Serial.begin(9600);  
  keypair.initialize(true);
}

void loop() {
    cnt++;
    if(cnt==5){ 
        
      try {
        std::vector<uint8_t> public_key = keypair.getPublicKey();
        Serial.println("Public key:");
        for(auto byte : public_key){
            Serial.print(byte, HEX);
        }
        Serial.println();
        Serial.println();
        /*
        std::vector<unsigned char> message = {'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
        std::vector<unsigned char> signature = keypair.sign(message);
          
        // Print signature
        printf("Signature: ");
        for (auto byte : signature) {
            printf("%02x", byte);
        }
        printf("\n");

        Keypair keypair2;
        bool is_verified = keypair2.verify(message, keypair.getPublicKey(), signature);
        if (is_verified) {
            printf("Signature verified\n");
        } else {
            printf("Signature verification failed\n");
        }
        */
      } catch (const std::exception& e) {
          printf("Exception: %s\n", e.what());
      }
      
      cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(5-cnt);
    }
    delay(1000);
}