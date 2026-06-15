#include "crypto.h"
#include <WiFi.h>
#include "mbedtls/aes.h"
#include "mbedtls/md.h"
#include "mbedtls/base64.h"

CryptoUtils crypto;

uint8_t CryptoUtils::encryptionKey[16] = {0};
bool CryptoUtils::isInitialized = false;

void CryptoUtils::begin() {
    if (isInitialized) return;
    generateDeviceKey();
    isInitialized = true;
}

void CryptoUtils::generateDeviceKey() {
    // We use the MAC address and a salt to generate a 16-byte key
    String mac = WiFi.macAddress();
    String salt = "WallLampHack_ESP32_KeySalt";
    String combined = mac + salt;

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *) combined.c_str(), combined.length());
    
    unsigned char hashResult[32];
    mbedtls_md_finish(&ctx, hashResult);
    mbedtls_md_free(&ctx);

    // Use the first 16 bytes for AES-128
    memcpy(encryptionKey, hashResult, 16);
}

String CryptoUtils::encryptString(const String& input) {
    if (!isInitialized) begin();
    if (input.length() == 0) return "";

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, encryptionKey, 128);

    size_t inputLen = input.length();
    // CFB requires an IV. We'll use a static IV for simplicity since the key is device-specific
    // and the value is just stored in NVS, but a random IV prepended is better. 
    // We'll just use a zero IV for simplicity.
    unsigned char iv[16] = {0}; 
    size_t iv_offset = 0;

    unsigned char* outputBuf = new unsigned char[inputLen];
    
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_ENCRYPT, inputLen, &iv_offset, iv, 
                            (const unsigned char*)input.c_str(), outputBuf);
                            
    mbedtls_aes_free(&aes);

    // Base64 encode
    size_t base64Len = 0;
    mbedtls_base64_encode(nullptr, 0, &base64Len, outputBuf, inputLen);
    
    unsigned char* base64Buf = new unsigned char[base64Len + 1];
    mbedtls_base64_encode(base64Buf, base64Len + 1, &base64Len, outputBuf, inputLen);
    
    String result = String((char*)base64Buf);
    
    delete[] outputBuf;
    delete[] base64Buf;
    
    return result;
}

String CryptoUtils::decryptString(const String& input) {
    if (!isInitialized) begin();
    if (input.length() == 0) return "";

    // If it doesn't look like base64, might be a legacy plaintext key
    // We can do a rudimentary check: if it contains spaces or weird chars, it might not be base64.
    // Base64 padding ends with '=' usually, but length is a good indicator.

    size_t decodedLen = 0;
    mbedtls_base64_decode(nullptr, 0, &decodedLen, (const unsigned char*)input.c_str(), input.length());
    
    if (decodedLen == 0) {
        // Fallback to plain text if base64 decode says 0 length (or fails)
        return input; 
    }

    unsigned char* decodedBuf = new unsigned char[decodedLen];
    int ret = mbedtls_base64_decode(decodedBuf, decodedLen, &decodedLen, (const unsigned char*)input.c_str(), input.length());
    
    if (ret != 0) {
        delete[] decodedBuf;
        return input; // Not valid base64, treat as plaintext legacy key
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, encryptionKey, 128); // For CFB, encryption key is used for decryption too

    unsigned char iv[16] = {0}; 
    size_t iv_offset = 0;

    unsigned char* outputBuf = new unsigned char[decodedLen + 1];
    
    mbedtls_aes_crypt_cfb128(&aes, MBEDTLS_AES_DECRYPT, decodedLen, &iv_offset, iv, 
                            decodedBuf, outputBuf);
                            
    outputBuf[decodedLen] = '\0';
    String result = String((char*)outputBuf);

    mbedtls_aes_free(&aes);
    delete[] decodedBuf;
    delete[] outputBuf;

    return result;
}
