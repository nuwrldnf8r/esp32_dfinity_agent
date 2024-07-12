#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

}

void loop(){
    mbedtls_ecdsa_context ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "ecdsa";

    mbedtls_ecdsa_init(&ctx);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    // Initialize random number generator
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                    (const unsigned char *) pers,
                                    strlen(pers));
    if (ret != 0) {
        // Handle error
    }

    // Generate the ECDSA key pair
    ret = mbedtls_ecdsa_genkey(&ctx, MBEDTLS_ECP_DP_SECP256K1,
                               mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        // Handle error
        Serial.println("Key pair unsuccessful");
    } else {
        Serial.println("Key pair successful");
    }

    // The key pair is now in ctx

    // Clean up
    mbedtls_ecdsa_free(&ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    delay(1000);
}