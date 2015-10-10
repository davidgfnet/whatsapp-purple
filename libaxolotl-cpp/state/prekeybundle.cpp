#include "prekeybundle.h"

PreKeyBundle::PreKeyBundle(uint64_t registrationId, int deviceId, uint64_t preKeyId, const DjbECPublicKey &preKeyPublic, uint64_t signedPreKeyId, const DjbECPublicKey &signedPreKeyPublic, const ByteArray &signedPreKeySignature, const IdentityKey &identityKey)
{
    this->registrationId        = registrationId;
    this->deviceId              = deviceId;
    this->preKeyId              = preKeyId;
    this->preKeyPublic          = preKeyPublic;
    this->signedPreKeyId        = signedPreKeyId;
    this->signedPreKeyPublic    = signedPreKeyPublic;
    this->signedPreKeySignature = signedPreKeySignature;
    this->identityKey           = identityKey;
}

int PreKeyBundle::getDeviceId() const
{
    return deviceId;
}

uint64_t PreKeyBundle::getPreKeyId() const
{
    return preKeyId;
}

DjbECPublicKey PreKeyBundle::getPreKey() const
{
    return preKeyPublic;
}

uint64_t PreKeyBundle::getSignedPreKeyId() const
{
    return signedPreKeyId;
}

DjbECPublicKey PreKeyBundle::getSignedPreKey() const
{
    return signedPreKeyPublic;
}

ByteArray PreKeyBundle::getSignedPreKeySignature() const
{
    return signedPreKeySignature;
}

IdentityKey PreKeyBundle::getIdentityKey() const
{
    return identityKey;
}

uint64_t PreKeyBundle::getRegistrationId() const
{
    return registrationId;
}
