#ifndef INMEMORYIDENTITYKEYSTORE_H
#define INMEMORYIDENTITYKEYSTORE_H

#include "state/identitykeystore.h"
#include "identitykeypair.h"
#include "util/keyhelper.h"

#include <map>

class InMemoryIdentityKeyStore : public IdentityKeyStore
{
public:
    InMemoryIdentityKeyStore();

    IdentityKeyPair getIdentityKeyPair();
    unsigned int    getLocalRegistrationId();
    void            saveIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    bool            isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    void storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair) { }
    void           removeIdentity(uint64_t recipientId) {
        trustedKeys.erase(recipientId);
    }

private:
    std::map<uint64_t, IdentityKey> trustedKeys;
    IdentityKeyPair identityKeyPair;
    uint64_t localRegistrationId;
};

#endif // INMEMORYIDENTITYKEYSTORE_H
