#include <Arduino.h>
#include <tinycbor.h>

int cnt = 0;
//const char* sender = "04";
//const char* canisterId = "1234";

#include <Arduino.h>
#include "tinycbor/cbor.h"

// Error check and exiting.
#define CHECK_ERROR(proc) {\
  if( (err = proc) != 0) {\
    err_line = __LINE__;\
    goto on_error;\
  }\
}

uint8_t encode_buffer[1024];

void setup() {
  Serial.begin(9600);
}

char* stringToHexString(const char* input) {
    const char hexDigits[] = "0123456789ABCDEF";
    size_t inputLength = strlen(input);
    size_t outputLength = inputLength * 2 + 1; // Each character will be represented by two hex digits plus null terminator

    char* hexString = (char*)malloc(outputLength);
    if (hexString == nullptr) {
        return nullptr; // Allocation failed
    }

    for (size_t i = 0; i < inputLength; ++i) {
        unsigned char c = input[i];
        hexString[i * 2]     = hexDigits[c >> 4];   // High nibble
        hexString[i * 2 + 1] = hexDigits[c & 0x0F]; // Low nibble
    }
    hexString[outputLength - 1] = '\0'; // Null terminator

    return hexString;
}

uint8_t* hexStringToBytes(const char* hexString) {
    // Calculate the length of the hexadecimal string
    size_t hexStringLength = strlen(hexString);

    // Calculate the length of the byte array (each byte represents two characters)
    size_t byteStringLength = hexStringLength / 2;

    // Allocate memory for the byte array
    uint8_t* byteArray = new uint8_t[byteStringLength];

    // Convert the hexadecimal string to bytes
    for (size_t i = 0; i < byteStringLength; ++i) {
        sscanf(hexString + 2 * i, "%2hhx", &byteArray[i]);
    }

    // Return the byte array
    return byteArray;
}

void encode( 
  const uint64_t ingress_expiry, 
  const char* sender, 
  const char* canisterId, 
  const char* request_type, 
  const char* method_name,
  const char* args
) {
  int err = 0;
  int err_line = 0;

  //uint ingress_expiry = 12345678901234567890;
  //uint64_t ingress_expiry = 1716978352862;

  // Initialize TinyCBOR library.
  TinyCBOR.init();
  // Assign buffer to encoder.
  TinyCBOR.Encoder.init(encode_buffer, sizeof(encode_buffer));
  /////////////////////////
  // Start CBOR encoding //
  /////////////////////////
  CHECK_ERROR(TinyCBOR.Encoder.create_map(1));
  {
    CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("content"));
    CHECK_ERROR(TinyCBOR.Encoder.create_map(6));
    {
      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("ingress_expiry"));
      CHECK_ERROR(TinyCBOR.Encoder.encode_uint(ingress_expiry));

      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("sender"));
      CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(hexStringToBytes(sender),strlen(sender)/2));

      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("canister_id"));
      CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(hexStringToBytes(canisterId),strlen(canisterId)/2));

      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("request_type"));
      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz(request_type));

      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("method_name"));
      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz(method_name));

      CHECK_ERROR(TinyCBOR.Encoder.encode_text_stringz("args"));
      char* params = stringToHexString(args);
      CHECK_ERROR(TinyCBOR.Encoder.encode_byte_string(hexStringToBytes(params),strlen(params)/2));
      free(params);

    }
    CHECK_ERROR(TinyCBOR.Encoder.close_container());
  }
  CHECK_ERROR(TinyCBOR.Encoder.close_container());
  ///////////////////////
  // End CBOR encoding //
  ///////////////////////

  {
    // Encoded data can retrieve with get_buffer() and get_buffer_size().
    uint8_t* buf = TinyCBOR.Encoder.get_buffer(); 
    size_t sz = TinyCBOR.Encoder.get_buffer_size();
    Serial.print("encoded data size = ");
    Serial.println(sz);
    Serial.println("output: ");
    for(uint32_t i=0; i<sz; i++) {
      if(i%8 == 0) {
        Serial.println();
        Serial.print("  ");
      }
      //Serial.print("0x");
      if(buf[i] < 0x10) Serial.print("0");
      Serial.print(buf[i], HEX);
      //if(i!=sz-1) Serial.print(", ");
    }
    Serial.println();
    err = 0;
  }
  on_error:
  // Detect error in CHECK_ERROR macro, jump to here.
  if(err) {
    Serial.print("Error: ");
    Serial.print(err);
    Serial.print(" raise at line:");
    Serial.print(err_line);
    Serial.println();
  }


}

void loop() {
  if(cnt==5){
    //Serial.println("Hex for 'hello':");
    //Serial.println(stringToHexString("hello"));
    encode(1716978352862,"04","1234","query","function","hello");
    cnt = 0;
  } else {
    Serial.println(5-cnt);
    cnt++;
  }
  delay(1000);
}
