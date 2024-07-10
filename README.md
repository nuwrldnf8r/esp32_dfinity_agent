# ESP32 HTTP Agent

This project is comprised of the following parts:
- `http_agent`
- `sketches/gateway_demo`
- `sketches/sensor_demo`
- `earthstream/src/earthstream_backend`
- `earthstream/src/earthstream_frontend`

## HTTP Agent

This is a work in progress and will be moved to its own repo. Written in C++ specifically for the ESP32, although the code can be extended as a more general C++ library. The functionality is currently catered for the Earthstream sensor gateway and, as such, is not a complete agent (covered in TODO).

- `http_agent.h`: Primarily encapsulates methods to update and query a canister on Internet Computer.
- `candid.h`: A minimalist Candid encoder and decoder for the purposes of Earthstream sensor data upload.
- `request.h`: Handles CBOR encoding for HTTP requests.
- `response.h`: Handles CBOR decoding for HTTP responses.
- `utils.h`: General functions - hashing, string to byte conversion, etc.
- `keypair.h`: A library for generating a Principal from a secp256k1 key pair, signing, etc.

### TODO
- More robust exception handling.
- `keypair.cpp` is using both micro-ecc (`uECC.h`) and mbedtls. It would be more efficient to just use mbedtls, but the version embedded in the ESP32 SDK differs from the documentation, so the API for signing and verifying was problematic. Micro-ecc is a workaround for now.
- The Candid library can be fleshed out a whole lot more.
- The functionality for calling a canister currently works but is incomplete as it still needs to do a `read_state` call. At the moment, it assumes after receiving a 202 that the data is uploaded, which is a fair assumption but not entirely confirmed.
- Delegation still needs to be included. As it wasn't necessary for this use case, we left it out for now.

## Gateway Demo

This is a sketch for the Earthstream gateway:
- WiFi SSID and password are set up through the serial port (initially, we were using Bluetooth, but the combination of Bluetooth, HTTP, agent, and LoRa libraries made the sketch too large). Settings are stored in the EEPROM, so they only need to be set up once and can be changed by the Principal who did the initial setup.
- Generates and stores a private key, which is used to sign HTTP packets sent to the IC.
- Registers itself with the Earthstream backend canister on the IC.
- Listens via LoRa to incoming messages from sensors.
- Validates and uploads data to the Earthstream backend canister on the IC.

### TODO
- More robust exception handling.
- Clean up the serial communication code (it works but can be neater and more robust).
- Switch between listening and communicating over LoRa and implement a protocol for two-way communication with sensors (including mesh networking).

## Sensor Demo

- Generates and stores a private key (used for signing data and public key serves as identification on the network).
- Currently has a temperature and humidity sensor, light intensity sensor, and GPS module (this will also include soil moisture and barometric pressure, as well as acoustic monitoring for later milestones).
- Signs and sends data (once a minute for demo purposes) via LoRa.

### TODO
This is a work in progress. The main feature will be bio-acoustic sensing and preprocessing, which is covered in a later milestone.

## Earthstream Backend

- Minimal canister to demonstrate uploading the data and registering the sensor on the network.

### TODO
This is a work in progress and much of this will be fleshed out in later milestones.

## Earthstream Frontend

This is a minimal web front-end to:
- Set up the gateway WiFi settings.
- Display uploaded data of the sensors in real-time.

### TODO
This wasn't really part of Milestone 1, but the gateway needed to be provisioned. Will form the backbone of the marketplace.
