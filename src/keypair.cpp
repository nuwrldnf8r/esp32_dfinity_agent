#include "keypair.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"

Keypair::Keypair(){
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "Keypair";

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Initialize random number generator
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));

    // Generate a random number
    unsigned char random_number[32];
    mbedtls_ctr_drbg_random(&ctr_drbg, random_number, sizeof(random_number));

    // Convert the random number to a string
    std::string random_number_str((char*)random_number, sizeof(random_number));

    // Pass the random number to the generate_key_pair function
    auto keypair = generate_key_pair(random_number_str);

    // Clean up
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
}

std::string Keypair::getRandom(){
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "Keypair";

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // Initialize random number generator
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));

    // Generate a random number
    unsigned char random_number[32];
    mbedtls_ctr_drbg_random(&ctr_drbg, random_number, sizeof(random_number));

    // Convert the random number to a string
    std::string random_number_str((char*)random_number, sizeof(random_number));

    // Clean up
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return random_number_str;
}

Keypair::Keypair(const std::string& private_random_number){
    _keypair = generate(private_random_number);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Keypair::generate(const std::string& private_random_number) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ecdsa_context ctx;

    const char *pers = "ecdsa";
    unsigned char public_key[200];
    unsigned char private_key[200];
    size_t olen;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ecdsa_init(&ctx);

    // Initialize random number generator with private_random_number
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, 
                          (const unsigned char *) private_random_number.c_str(), 
                          private_random_number.size());

    // Generate key pair
    mbedtls_ecdsa_genkey(&ctx, MBEDTLS_ECP_DP_SECP256R1, mbedtls_ctr_drbg_random, &ctr_drbg);

    // Write public key to public_key
    mbedtls_ecp_point_write_binary(&ctx.grp, &ctx.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, public_key, sizeof(public_key));

    // Write private key to private_key
    mbedtls_mpi_write_binary(&ctx.d, private_key, mbedtls_mpi_size(&ctx.d));

    // Clean up
    mbedtls_ecdsa_free(&ctx);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    // Convert the public and private keys to std::vector<uint8_t> and return them
    std::vector<uint8_t> public_key_vec(public_key, public_key + olen);
    std::vector<uint8_t> private_key_vec(private_key, private_key + mbedtls_mpi_size(&ctx.d));
    return std::make_pair(public_key_vec, private_key_vec);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Keypair::generate(const std::vector<uint8_t>& private_key_vec) {
    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);

    // Convert private key to mbedtls_mpi
    mbedtls_mpi private_key_mpi;
    mbedtls_mpi_init(&private_key_mpi);
    mbedtls_mpi_read_binary(&private_key_mpi, private_key_vec.data(), private_key_vec.size());

    // Set the group (curve) to use
    mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);

    // Generate the public key
    mbedtls_ecp_mul(&ctx.grp, &ctx.Q, &private_key_mpi, &ctx.grp.G, NULL, NULL);

    // Write public key to public_key
    unsigned char public_key[200];
    size_t olen;
    mbedtls_ecp_point_write_binary(&ctx.grp, &ctx.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, public_key, sizeof(public_key));

    // Clean up
    mbedtls_mpi_free(&private_key_mpi);
    mbedtls_ecdsa_free(&ctx);

    // Convert the public and private keys to std::vector<uint8_t> and return them
    std::vector<uint8_t> public_key_vec(public_key, public_key + olen);
    return std::make_pair(public_key_vec, private_key_vec);
}

void keypair::generate(const std::string& private_random_number){
    _keypair = generate(private_random_number);
}

void keypair::generate(const std::vector<uint8_t>& private_key_vec){
    _keypair = generate(private_key_vec);
}
