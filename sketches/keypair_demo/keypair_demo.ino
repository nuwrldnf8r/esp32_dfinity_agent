#include "keypair/keypair.h"
#include "keypair/keypair.cpp"
#include <stdexcept>
#include "esp_system.h"

int cnt = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);  
}

void loop() {
    cnt++;
    if(cnt==3){
        
      try {
        Keypair keypair = Keypair();
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

      } catch (const std::exception& e) {
          printf("Exception: %s\n", e.what());
      }

        cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(3-cnt);
    }
    delay(1000);
}