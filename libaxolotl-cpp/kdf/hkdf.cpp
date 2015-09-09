#include "hkdf.h"
#include "byteutil.h"
#include <cmath>
#include <algorithm>
#include <openssl/hmac.h>
#include <openssl/sha.h>

const float HKDF::HASH_OUTPUT_SIZE = 32;

HKDF::HKDF(int messageVersion)
{
    iterationStartOffset = 0;
    if (messageVersion == 2) {
        iterationStartOffset = 0;
    }
    else if (messageVersion == 3) {
        iterationStartOffset = 1;
    }
    else {
        // TODO exception
    }
}

int HKDF::getIterationStartOffset() const
{
    return iterationStartOffset;
}

ByteArray HKDF::expand(const ByteArray &prk, const ByteArray &info, int outputSize) const
{
    int iterations = std::ceil((float)outputSize / HKDF::HASH_OUTPUT_SIZE);
    ByteArray mixin;
    ByteArray results;
    int remainingBytes = outputSize;

    for (int i = iterationStartOffset; i < (iterations + iterationStartOffset); i++) {

        ByteArray message(mixin);
        if (!info.empty()) {
            message += info;
        }

        message += ByteArray(1, (char)(i % 256));

        unsigned char out[EVP_MAX_MD_SIZE];
        unsigned int outlen;
        HMAC(EVP_sha256(), prk.c_str(), prk.size(), (unsigned char*)message.c_str(), message.size(), out, &outlen);
        ByteArray stepResult((const char*)out, outlen);

        int stepSize = std::min((int)remainingBytes, (int)stepResult.size());
        results += stepResult.substr(0, stepSize);
        mixin = stepResult;
        remainingBytes -= stepSize;
    }
    return results;
}

ByteArray HKDF::extract(const ByteArray &salt, const ByteArray &inputKeyMaterial) const
{
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int outlen;
    HMAC(EVP_sha256(), salt.c_str(), salt.size(), (unsigned char*)inputKeyMaterial.c_str(), inputKeyMaterial.size(), out, &outlen);
    return ByteArray((const char*)out, outlen);
}

ByteArray HKDF::deriveSecrets(const ByteArray &inputKeyMaterial, const ByteArray &info, int outputLength, const ByteArray &saltFirst) const
{
    ByteArray salt = saltFirst;
    if (salt.empty()) {
        salt = ByteArray(HKDF::HASH_OUTPUT_SIZE, '\0');
    }
    ByteArray prk = extract(salt, inputKeyMaterial);
    return expand(prk, info, outputLength);
}
