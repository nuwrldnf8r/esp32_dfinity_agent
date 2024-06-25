#include "agent/candid.h"
#include "agent/candid.cpp"

void setup(){
    Serial.begin(9600);

}

void loop(){
    Parameter p1 = Parameter(true);
    Parameter p2 = Parameter((int64_t)42);
    printf("hitting::");
    Parameter p3 = Parameter((std::string)"Hello World");
    std::vector<uint8_t> v = {'a', 'b', 'c'};
    Parameter p4 = Parameter(v);

    
    std::string s = p3.getType();
    Serial.println("***********");
    Serial.println(s.c_str());
    

    Serial.println("Creating parameters:");
    std::vector<Parameter> params = {p1,p2,p3,p4};
    

    Serial.println("Creating candid object:");

    Candid c = Candid(params);
    
    Serial.println("Decoding parameters:");
    std::vector<Parameter> decoded = c.decode();
    Serial.println("Printing parameters:");
    for(auto p : decoded){
            Serial.print("Type: ");
            Serial.println(p.getType().c_str());
            Serial.print("Value: ");
            if(p.getType() == "bool"){
                Serial.println("Parsing bool");
                Serial.println(p.parseBool());
            } else if(p.getType() == "int"){
                Serial.println(p.parseInt());
            } else if(p.getType() == "text"){
                Serial.println(p.parseText().c_str());
            } else if(p.getType() == "blob"){
                
                std::vector<uint8_t> blob = p.parseBlob();
                for(auto b : blob){
                    Serial.print((char)b);
                }
                Serial.println();
                
            }
    }
    
    delay(5000);
}