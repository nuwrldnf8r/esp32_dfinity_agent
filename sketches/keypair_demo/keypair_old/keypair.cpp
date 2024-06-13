#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <stdexcept>
#include <stdint.h>
#include <stddef.h>

// This function assumes that the private key is a DER-encoded ASN.1 integer.
// It does not handle other DER formats.
int parse_der_private_key(const uint8_t *der_key, size_t der_key_len, uint8_t *out_key) {
    // Check that the key is at least 3 bytes long (to hold the type, length, and at least one byte of data).
    if (der_key_len < 3) {
        return -1;
    }
    // Check that the key is an ASN.1 integer.
    if (der_key[0] != 0x02) {
        return -1;
    }
    // Get the length of the integer.
    size_t len = der_key[1];
    // Check that the length is valid.
    if (len > der_key_len - 2) {
        return -1;
    }
    // Copy the integer to the output buffer.
    for (size_t i = 0; i < len; i++) {
        out_key[i] = der_key[i + 2];
    }
    return 0;
}



Keypair::Keypair(){
    int ret = 0;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_pk_context pk;
    const char* pers = "ecdsa_keygen";

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_pk_init(&pk);

    // Seed the random number generator
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,reinterpret_cast<const unsigned char*>(pers), strlen(pers))) != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        throw std::runtime_error("Failed to seed the random number generator");
    }

    // Initialize the context for the public key algorithm
    if ((ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY))) != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        throw std::runtime_error("Failed to initialize the public key context");
    }

    // Generate the key pair on the specified curve (secp256k1)
    if ((ret = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256K1, mbedtls_pk_ec(pk),
                                   mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
        throw std::runtime_error("Failed to generate the key pair");
    }

    // Save private key to DER format
    std::vector<unsigned char> private_key_buf(16000);
    ret = mbedtls_pk_write_key_der(&pk, private_key_buf.data(), private_key_buf.size());
    if (ret < 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
         throw std::runtime_error("Failed to write the private key to DER format");
    }
    unsigned char* derPrivateKey = private_key_buf.data() + private_key_buf.size() - ret;
    size_t len = ret;
    _private_key_buf.resize(len);
    std::copy(derPrivateKey, derPrivateKey + len, _private_key_buf.begin());

   

    // Save public key to DER format
    std::vector<unsigned char> public_key_buf(16000);
    ret = mbedtls_pk_write_pubkey_der(&pk, public_key_buf.data(), public_key_buf.size());
    if (ret < 0) {
        mbedtls_pk_free(&pk);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
         throw std::runtime_error("Failed to write the public key to DER format");
    }
    unsigned char* derPublicKey = public_key_buf.data() + public_key_buf.size() - ret;
    len = ret;
    _public_key_buf.resize(len);
    std::copy(derPublicKey, derPublicKey + len, _public_key_buf.begin());

    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

}



Keypair::Keypair(const std::vector<unsigned char>& private_key_buf){
     _private_key_buf.resize(private_key_buf.size());
    std::copy(private_key_buf.begin(), private_key_buf.end(), _private_key_buf.begin());

    // Parse the private key
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    int ret = mbedtls_pk_parse_key(&pk, _private_key_buf.data(), _private_key_buf.size(), nullptr, 0, nullptr, nullptr);    if (ret != 0) {
        mbedtls_pk_free(&pk);
        throw std::runtime_error("Failed to parse the private key");
    }

    // Write the public key to a buffer in DER format
    std::vector<unsigned char> public_key_buf(16000);
    ret = mbedtls_pk_write_pubkey_der(&pk, public_key_buf.data(), public_key_buf.size());
    if (ret < 0) {
        mbedtls_pk_free(&pk);
        throw std::runtime_error("Failed to write the public key to a buffer");
    }

    // mbedtls_pk_write_pubkey_der writes to the end of the buffer, so we need to adjust the pointer and length
    unsigned char* derPublicKey = public_key_buf.data() + public_key_buf.size() - ret;
    size_t len = ret;

    // Resize and copy the public key to _public_key_buf
    _public_key_buf.resize(len);
    std::copy(derPublicKey, derPublicKey + len, _public_key_buf.begin());

    mbedtls_pk_free(&pk);

}


std::vector<unsigned char> Keypair::sign(const std::vector<unsigned char>& message) {
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "ecdsa";
    int ret;

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        throw std::runtime_error("Failed to initialize ctr_drbg");
    }

    // Parse the private key
    ret = mbedtls_pk_parse_key(&pk, _private_key_buf.data(), _private_key_buf.size(), nullptr, 0, nullptr, nullptr);
    if (ret != 0) {
        throw std::runtime_error("Failed to parse the private key");
    }

    // Make sure the key is an EC key
    if (!mbedtls_pk_can_do(&pk, MBEDTLS_PK_ECKEY)) {
        throw std::runtime_error("The key is not an EC key");
    }

    // Get the EC key
    mbedtls_ecp_keypair* ec = mbedtls_pk_ec(pk);

    // Generate the signature
    unsigned char sig[2 * 32 + 9];  // Make sure this is large enough
    size_t sig_len; // = sizeof(sig);
    //size_t msg_size = message.size();
    ret = mbedtls_ecdsa_write_signature(ec, MBEDTLS_MD_SHA256, message.data(), 32, sig, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);    
    if (ret != 0) {
        throw std::runtime_error("Failed to generate the signature");
    }

    // Copy the signature to a vector
    std::vector<unsigned char> signature(sig, sig + sig_len);

    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return signature;
}

