#include "mbedtls/ecdsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

// Generate a new ECDSA key pair
void generate_keypair(mbedtls_ecdsa_context *ctx) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "ecdsa";

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random number generator
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));

    // Generate the keypair
    mbedtls_ecdsa_genkey(ctx, MBEDTLS_ECP_DP_SECP256K1, mbedtls_ctr_drbg_random, &ctr_drbg);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

// Sign a message
int sign_message(mbedtls_ecdsa_context *ctx, const unsigned char *message, size_t message_len, unsigned char *sig, size_t *sig_len) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "ecdsa";

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Seed the random number generator
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));

    // Sign the message
    int ret = mbedtls_ecdsa_write_signature(ctx, MBEDTLS_MD_SHA256, message, message_len, sig, sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return ret;
}

// Verify a signature
int verify_signature(mbedtls_ecdsa_context *ctx, const unsigned char *message, size_t message_len, const unsigned char *sig, size_t sig_len) {
    // Verify the signature
    int ret = mbedtls_ecdsa_read_signature(ctx, message, message_len, sig, sig_len);

    return ret;
}

int get_private_key(mbedtls_ecdsa_context *ctx, unsigned char *buf, size_t buf_len) {
    return mbedtls_mpi_write_binary(&ctx->d, buf, buf_len);
}

int load_private_key(mbedtls_ecdsa_context *ctx, const unsigned char *private_key, size_t private_key_len) {
    return mbedtls_mpi_read_binary(&ctx->d, private_key, private_key_len);
}

int get_public_key(mbedtls_ecdsa_context *ctx, unsigned char *buf, size_t *buf_len) { // Make buf_len a pointer
    return mbedtls_ecp_point_write_binary(&ctx->grp, &ctx->Q, MBEDTLS_ECP_PF_UNCOMPRESSED, buf_len, buf, *buf_len);
}

int load_public_key(mbedtls_ecdsa_context *ctx, const unsigned char *public_key, size_t public_key_len) {
    return mbedtls_ecp_point_read_binary(&ctx->grp, &ctx->Q, public_key, public_key_len);
}

void setup(){

  Serial.begin(9600);

}

void loop(){
    unsigned char public_key[65];
    

    mbedtls_ecdsa_context ctx2;
    mbedtls_ecdsa_init(&ctx2);
    mbedtls_ecp_group_load(&ctx2.grp, MBEDTLS_ECP_DP_SECP256K1);

    // Generate a new key pair
    generate_keypair(&ctx2);
    size_t public_key_len = sizeof(public_key);
    int ret = get_public_key(&ctx2, public_key, &public_key_len);
    if (ret != 0) {
        printf("Failed to get public key\n");
    } else {
        printf("Public key obtained successfully: ");
        for (size_t i = 0; i < public_key_len; i++) {
            printf("%02x", public_key[i]);
        }
        printf("\n");
    }
    unsigned char message[] = "Hello, world!";
    unsigned char sig[100];
    size_t sig_len;
    sign_message(&ctx2, message, strlen((char*)message), sig, &sig_len);

    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);
    mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256K1);
    
    ret = load_public_key(&ctx, public_key, public_key_len);
    if (ret != 0) {
        printf("Failed to load public key into new context\n");
    } else {
        printf("Public key loaded into new context successfully\n");
    }    

    /*
    // Sign a message
    unsigned char message[] = "Hello, world!";
    unsigned char sig[100];
    size_t sig_len;
    sign_message(&ctx, message, sizeof(message), sig, &sig_len);
    */

    // Verify the signature
    ret = verify_signature(&ctx, message, strlen((char*)message), sig, sig_len);
    if (ret == 0) {
        printf("Signature verified\n");
    } else {
        printf("Signature verification failed\n");
    }

    mbedtls_ecdsa_free(&ctx);
    mbedtls_ecdsa_free(&ctx2);
    delay(5000);
}