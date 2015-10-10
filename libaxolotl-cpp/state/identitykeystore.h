#ifndef IDENTITYKEYSTORE_H
#define IDENTITYKEYSTORE_H

#include "identitykeypair.h"

class IdentityKeyStore {
public:
    virtual IdentityKeyPair getIdentityKeyPair() = 0;
    virtual unsigned int    getLocalRegistrationId() = 0;
    virtual void            storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair) = 0;
    virtual void            saveIdentity(uint64_t recipientId, const IdentityKey &identityKey) = 0;
    virtual bool            isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey) = 0;
    virtual void            removeIdentity(uint64_t recipientId) = 0;
};

#endif // IDENTITYKEYSTORE_H
