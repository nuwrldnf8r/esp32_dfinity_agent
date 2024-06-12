#include "keypair.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include <sstream>
#include <iomanip>

std::string bytesToHexString(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;
    for(const auto& byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

std::vector<uint8_t> stringToBytes(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::vector<uint8_t> hexStringToBytes(const std::string& str) {
    std::vector<uint8_t> bytes;
    for (unsigned int i = 0; i < str.length(); i += 2) {
        std::string byteString = str.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

Keypair::Keypair(){
    mbedtls_entropy_context entropy;
    //mbedtls_ctr_drbg_context ctr_drbg;
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
    auto keypair = generateKeyPair(random_number_str);

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
    _keypair = generateKeyPair(private_random_number);
}

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Keypair::generateKeyPair(const std::string& private_random_number) {
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

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> Keypair::generateKeyPair(const std::vector<uint8_t>& private_key_vec) {
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

void Keypair::generate(const std::string& private_random_number){
    _keypair = generateKeyPair(private_random_number);
}

void Keypair::generate(const std::vector<uint8_t>& private_key_vec){
    _keypair = generateKeyPair(private_key_vec);
}

std::vector<uint8_t> Keypair::getPublicKey(){
    return _keypair.first;
}

std::vector<uint8_t> Keypair::getPrivateKey(){
    return _keypair.second;
}   

std::string Keypair::getPublicKeyString(){
    return bytesToHexString(_keypair.first);
}

std::string Keypair::getPrivateKeyString(){
    return bytesToHexString(_keypair.second);
}

std::vector<uint8_t> Keypair::sign(const std::vector<uint8_t>& message){
    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);

    // Convert private key to mbedtls_mpi
    mbedtls_mpi private_key_mpi;
    mbedtls_mpi_init(&private_key_mpi);
    mbedtls_mpi_read_binary(&private_key_mpi, _keypair.second.data(), _keypair.second.size());

    // Set the group (curve) to use
    mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);

    // Set the private key
    mbedtls_mpi_copy(&ctx.d, &private_key_mpi);

    // Sign the message
    unsigned char signature[200];
    size_t olen;
    mbedtls_ecdsa_write_signature(&ctx, MBEDTLS_MD_SHA256, message.data(), message.size(), signature, &olen, mbedtls_ctr_drbg_random, &ctr_drbg);

    // Clean up
    mbedtls_mpi_free(&private_key_mpi);
    mbedtls_ecdsa_free(&ctx);

    // Convert the signature to std::vector<uint8_t> and return it
    std::vector<uint8_t> signature_vec(signature, signature + olen);
    return signature_vec;
}

bool Keypair::verify(const std::vector<uint8_t>& message, const std::vector<uint8_t>& signature){
    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);

    // Set the group (curve) to use
    mbedtls_ecp_group_load(&ctx.grp, MBEDTLS_ECP_DP_SECP256R1);

    // Convert public key to mbedtls_ecp_point
    mbedtls_ecp_point public_key_point;
    mbedtls_ecp_point_init(&public_key_point);
    mbedtls_ecp_point_read_binary(&ctx.grp, &public_key_point, _keypair.first.data(), _keypair.first.size());

    // Set the public key
    mbedtls_ecp_copy(&ctx.Q, &public_key_point);

    // Verify the signature
    int ret = mbedtls_ecdsa_read_signature(&ctx, message.data(), message.size(), signature.data(), signature.size());

    // Clean up
    mbedtls_ecp_point_free(&public_key_point);
    mbedtls_ecdsa_free(&ctx);

    // Return true if the signature is valid, false otherwise
    return ret == 0;
}

std::vector<uint8_t> Keypair::sign(const std::string& message){
    std::vector<uint8_t> message_vec = stringToBytes(message);
    std::vector<uint8_t> signature_vec = sign(message_vec);
    return signature_vec;
}

bool Keypair::verify(const std::string& message, const std::vector<uint8_t>& signature){
    std::vector<uint8_t> message_vec = stringToBytes(message);
    return verify(message_vec, signature);
}
// Path: src/main.cpp