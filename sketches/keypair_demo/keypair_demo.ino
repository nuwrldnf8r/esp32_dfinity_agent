#include "keypair/keypair.h"
#include "keypair/keypair.cpp"
#include <stdexcept>

int cnt = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);  
}

void loop() {
    cnt++;
    if(cnt==3){
        try{
          Keypair keypair = Keypair();
          Keypair keypair2 = Keypair(keypair.private_key());
          std::vector<unsigned char> private_key = keypair2.private_key();
          Serial.println("Private key: ");
          for (unsigned char byte : private_key) {
            Serial.printf("%02x", byte);
          }
          Serial.println();
          Serial.println();
          std::vector<unsigned char> public_key = keypair.public_key();
          Serial.println("Public key 1: ");
          for (unsigned char byte : public_key) {
            Serial.printf("%02x", byte);
          }
          Serial.println();
          Serial.println();
          public_key = keypair2.public_key();
          Serial.println("Public key 2: ");
          for (unsigned char byte : public_key) {
            Serial.printf("%02x", byte);
          }
          Serial.println();
        } catch(const std::runtime_error& e){
          Serial.println(e.what());
        }
        

        cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(3-cnt);
    }
    delay(1000);
}