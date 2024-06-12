#include <mbedtls/ecdsa.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <stdexcept>

/*
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
*/



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