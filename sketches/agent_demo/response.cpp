#import "response.h"
#include <tinycbor.h>
#include <string>
#include <vector>
#include <cstdint>

Response::Response() {}

Response::Response(const std::vector<uint8_t>& data) {
    CborParser parser;
    CborValue it;

    CborError err = cbor_parser_init(data.data(), data.size(), 0, &parser, &it);
    if (err != CborNoError) {
        // Handle the error
    }

    // If the current item is a tag, skip over it
    if (cbor_value_get_type(&it) == CborTagType) {
        err = cbor_value_advance(&it);
        if (err != CborNoError) {
            // Handle the error
        }
    }

    CborValue map;

    err = cbor_value_enter_container(&it, &map);
    if (err != CborNoError) {
        // Handle the error
    }

    while (!cbor_value_at_end(&map)) {
        char* key;
        size_t keylen;
        err = cbor_value_dup_text_string(&map, &key, &keylen, NULL);
        if (err != CborNoError) {
            // Handle the error
        }

        err = cbor_value_advance(&map);
        if (err != CborNoError) {
            // Handle the error
        }

        if (strcmp(key, "status") == 0) {
            char* status_;
            size_t statuslen;
            err = cbor_value_dup_text_string(&map, &status_, &statuslen, NULL);
            if (err != CborNoError) {
                // Handle the error
            }
            status = status_;
            free(status_);

        } else if (strcmp(key, "reply") == 0) {
            
            if (cbor_value_get_type(&map) == CborMapType) {
                CborValue reply_map;
                err = cbor_value_enter_container(&map, &reply_map);
                if (err != CborNoError) {
                    // Handle the error
                    printf("Error entering map - 120\n");
                }

                while (!cbor_value_at_end(&reply_map)) {
                    char* reply_key;
                    size_t reply_keylen;
                    err = cbor_value_dup_text_string(&reply_map, &reply_key, &reply_keylen, NULL);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error getting key - 129\n");
                    }

                    err = cbor_value_advance(&reply_map);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error advancing - 134\n");
                    }

                    if (strcmp(reply_key, "arg") == 0) {
                        if (cbor_value_get_type(&reply_map) == CborByteStringType) {
                            size_t arglen;
                            err = cbor_value_calculate_string_length(&reply_map, &arglen);
                            if (err != CborNoError) {
                                // Handle the error
                                printf("Error calculating length - 138\n");
                            }
                            std::vector<uint8_t> arg(arglen);
                            err = cbor_value_copy_byte_string(&reply_map, arg.data(), &arglen, NULL);
                            if (err != CborNoError) {
                                // Handle the error
                                printf("Error getting arg - 145\n");
                            }
                            reply.arg = std::move(arg);
                        } else {
                            // Handle the error
                            printf("Error: expected a byte string\n");
                        }
                    }

                    free(reply_key);
                    err = cbor_value_advance(&reply_map);
                    if (err != CborNoError) {
                        // Handle the error
                        printf("Error advancing - 149\n");
                    }
                }
                
            } else {
                // Handle the error
                printf("Error getting type - 147\n");
            }
        } else if (strcmp(key, "signatures") == 0) {
            // Handle the "signatures" field
        }

        free(key);
        err = cbor_value_advance(&map);
        if (err != CborNoError) {
            // Handle the error
        }
    }
}