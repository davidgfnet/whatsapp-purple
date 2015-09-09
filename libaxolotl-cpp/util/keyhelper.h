#ifndef KEYHELPER_H
#define KEYHELPER_H

#include "byteutil.h"
#include "identitykeypair.h"
#include "prekeyrecord.h"
#include "signedprekeyrecord.h"

#include <vector>

class KeyHelper
{
public:
    static uint64_t getRandomFFFF();
    static uint64_t getRandom7FFFFFFF();
    static uint64_t getRandomFFFFFFFF();
    static ByteArray getRandomBytes(int bytes);

    static IdentityKeyPair generateIdentityKeyPair();
    static uint64_t generateRegistrationId();
    static std::vector<PreKeyRecord> generatePreKeys(uint64_t start, unsigned int count);
    static SignedPreKeyRecord generateSignedPreKey(const IdentityKeyPair &identityKeyPair, uint64_t signedPreKeyId);
    static ECKeyPair generateSenderSigningKey();
    static ByteArray generateSenderKey();
    static unsigned long generateSenderKeyId();
};

#endif // KEYHELPER_H
