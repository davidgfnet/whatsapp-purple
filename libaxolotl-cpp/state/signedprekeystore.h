#ifndef SIGNEDPREKEYSTORE_H
#define SIGNEDPREKEYSTORE_H

#include "signedprekeyrecord.h"
#include <vector>

class SignedPreKeyStore
{
public:
    virtual SignedPreKeyRecord loadSignedPreKey(uint64_t signedPreKeyId) = 0;
    virtual std::vector<SignedPreKeyRecord> loadSignedPreKeys() = 0;
    virtual void storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record) = 0;
    virtual bool containsSignedPreKey(uint64_t signedPreKeyId) = 0;
    virtual void removeSignedPreKey(uint64_t signedPreKeyId) = 0;
};

#endif // SIGNEDPREKEYSTORE_H
