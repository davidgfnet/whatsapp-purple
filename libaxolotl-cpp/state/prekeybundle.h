#ifndef PREKEYBUNDLE_H
#define PREKEYBUNDLE_H

#include "djbec.h"
#include "identitykey.h"
#include "byteutil.h"

class PreKeyBundle
{
public:
    PreKeyBundle(uint64_t registrationId, int deviceId, uint64_t preKeyId, const DjbECPublicKey &preKeyPublic, uint64_t signedPreKeyId, const DjbECPublicKey &signedPreKeyPublic, const ByteArray &signedPreKeySignature, const IdentityKey &identityKey);

    int getDeviceId() const;
    uint64_t getPreKeyId() const;
    DjbECPublicKey getPreKey() const;
    uint64_t getSignedPreKeyId() const;
    DjbECPublicKey getSignedPreKey() const;
    ByteArray getSignedPreKeySignature() const;
    IdentityKey getIdentityKey() const;
    uint64_t getRegistrationId() const;

private:
    uint64_t registrationId;
    int deviceId;
    uint64_t preKeyId;
    DjbECPublicKey preKeyPublic;
    uint64_t signedPreKeyId;
    DjbECPublicKey signedPreKeyPublic;
    ByteArray signedPreKeySignature;
    IdentityKey identityKey;

};

#endif // PREKEYBUNDLE_H
