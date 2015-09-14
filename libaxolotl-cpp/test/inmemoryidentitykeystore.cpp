#include "inmemoryidentitykeystore.h"

#include "ecc/curve.h"
#include "ecc/eckeypair.h"

#include <iostream>

InMemoryIdentityKeyStore::InMemoryIdentityKeyStore()
{
    ECKeyPair identityKeyPairKeys = Curve::generateKeyPair();
    identityKeyPair = IdentityKeyPair(IdentityKey(identityKeyPairKeys.getPublicKey()),
                                      identityKeyPairKeys.getPrivateKey());
    localRegistrationId = KeyHelper::generateRegistrationId();
}

IdentityKeyPair InMemoryIdentityKeyStore::getIdentityKeyPair()
{
    return identityKeyPair;
}

unsigned int InMemoryIdentityKeyStore::getLocalRegistrationId()
{
    return localRegistrationId;
}

void InMemoryIdentityKeyStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    trustedKeys[recipientId] = identityKey;
}

bool InMemoryIdentityKeyStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    if (trustedKeys.find(recipientId) == trustedKeys.end()) {
        return true;
    }
    return trustedKeys[recipientId] == identityKey;
}
