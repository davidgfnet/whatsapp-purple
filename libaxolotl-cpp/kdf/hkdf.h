#ifndef HKDF_H
#define HKDF_H

#include "byteutil.h"

class HKDF
{
public:
    HKDF(int messageVersion = 2);
    static const float HASH_OUTPUT_SIZE;
    int getIterationStartOffset() const;
    ByteArray expand(const ByteArray &prk, const ByteArray &info, int outputSize) const;
    ByteArray extract(const ByteArray &salt, const ByteArray &inputKeyMaterial) const;
    ByteArray deriveSecrets(const ByteArray &inputKeyMaterial, const ByteArray &info, int outputLength, const ByteArray &saltFirst = ByteArray()) const;

private:
    int iterationStartOffset;

};

#endif // HKDF_H
