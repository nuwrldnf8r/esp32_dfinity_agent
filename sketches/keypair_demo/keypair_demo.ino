#include "keypair/keypair.h"
#include "keypair/keypair.cpp"

int cnt = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);  
}

void loop() {
    cnt++;
    if(cnt==10){
        Keypair keypair;
        std::string random_number = keypair.getRandom();
        Keypair keypair2(random_number);
        Serial.println("Random number: ");
        Serial.println(random_number.c_str());
        Serial.println("Public key: ");
        for(auto i : keypair2.getPublicKey()){
            Serial.print(i, HEX);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println("Private key: ");
        for(auto i : keypair2.getPrivateKey()){
            Serial.print(i, HEX);
            Serial.print(" ");
        }

        std::string str = "Hello, world!";
        std::vector<uint8_t> vec(str.begin(), str.end());

        std::vector<uint8_t> sig = keypair2.sign(vec);
        bool isVerified = keypair2.verify(vec, sig);
        if(isVerified){
            Serial.println("Signature verified!");
        } else {
            Serial.println("Signature not verified!");
        }
        cnt = 0;
    } else {
        Serial.print("fetching in ");
        Serial.println(10-cnt);
    }
    delay(1000);
}