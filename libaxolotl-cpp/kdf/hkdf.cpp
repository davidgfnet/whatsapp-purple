#include "hkdf.h"
#include "byteutil.h"
#include <cmath>
#include <algorithm>
#include <iostream>

void HMAC_SHA256(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest);

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

		unsigned char out[32];
		HMAC_SHA256((unsigned char*)message.c_str(), message.size(), (unsigned char*)prk.c_str(), prk.size(), out);
        ByteArray stepResult((const char*)out, 32);

        int stepSize = std::min((int)remainingBytes, (int)stepResult.size());
        results += stepResult.substr(0, stepSize);
        mixin = stepResult;
        remainingBytes -= stepSize;
    }
    return results;
}

ByteArray HKDF::extract(const ByteArray &salt, const ByteArray &inputKeyMaterial) const
{
    unsigned char out[32];
	HMAC_SHA256((unsigned char*)inputKeyMaterial.c_str(), inputKeyMaterial.size(), (unsigned char*)salt.c_str(), salt.size(), out);
    return ByteArray((const char*)out, 32);
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
