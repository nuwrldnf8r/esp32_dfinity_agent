# ESP32 HTTP AGENT

This project is comprised of the following parts:
- http_agent
- sketches/gateway_demo
- sketches/sensor_demo
- earthstream/src/earthstream_backend
- earthstream/src/earthstream_frontend

## HTTP Agent:

This is a work in progress, and will be moved to it's own repo. Written in c++ specifically for the ESP32, although the code can be extended as a more general c++ library. The functionality is currently catered for the Earthsteam sensor gateway, and as such is not a complete Agent (covered in TODO).

- http_agent.h: Primarily encapsulates methods to update and query a canister on Internet Computer
- candid.h: A minimalist Candid encoder and decoder for the purposes of Earthstream sensor data upload. 
- request: Handles CBOR encoding for http requests
- response: Handles CBOR decoding for http responses
- utils: general functions - hashing etc, string to byte conversion etc.
- keypair: a library for generating a Principal from a secp256k1 key pair, signing etc.

TODO:
- more robust exception handling
- keypair.cpp is using both micro-ecc (uECC.h) and mbedtls. It would be more efficient to just use mbedtls, but the version embedded on the esp32 SDK differs from the documentation, so the API for signing and verifying were problematic. micro-ecc is a workaround for now.
- the candid library can be fleshed out a whole lot more
- the functionality for calling a canister currently works, but is incomplete as it needs to still do a read_state call. At the moment it assumes after receiving a 202 that the data is uploaded, which is a fair assumption, but not entirely confirmed
- delegation still needs to be included. As it wasn't necesarry for this use case we left it out for now

   
## Gateway Demo:
  
This a sketch for the Earthstream gateway:
- Wifi ssid and password are set up through the serial port (initially we were using bluetooth but the combination of bluetooth, http, agent and LoRa libraries made the sketch too large). Settings are stored in the EEPROM, so only need to be set up once, and can be changed by the Principal who did the initial setup
- generates and stores a private key - used to sign http packets sent to the IC
- registers itself with the Earthstream backend canister on the IC
- listens via LoRa to incoming messages from sensors
- validates and uploads data to the Earthstream backend canister on the IC
  
  
TODO:
- more robust exception handling
- clean up the serial communication code (it works, but can be neater and more robust)
- switch between listening and communicating over LoRa and implement a protocol for two way communication with sensors (including mesh networking)
  
  
## Sensor Demo
  
- generates and stores a private key (used for signing data and public key serves as identification on the network)
- currently has a temperature and humidity sensor, light intensity sensor and GPS module (this will also include soil moisture and barometric pressure, as well as acoustic monitoring for later milestones)
- signs and sends data (once a minute for demo purposes) via LoRa
  
  
TODO: This is a work in progress. The main feature will be bio-acoustic sensing and preprocessing, which is covered in a later milestone)

## Earthstream Backend

- minimal canister to demonstrate uploading the data and registering the sensor on the network
  
  
TODO: This is a work in progress and much of this will be fleshed out in later milestones

## Earthstream Frontend

This is a minimal web front-end to:
- set up the gateway wifi settings
- display uploaded data of the sensors in real-time
  
  
TODO: This wasn't really part of milestone 1, but the gateway needed to be provisioned. Will form the backbone of the marketplace



