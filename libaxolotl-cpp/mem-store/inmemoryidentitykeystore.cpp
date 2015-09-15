#include "inmemoryidentitykeystore.h"

#include "ecc/curve.h"
#include "ecc/eckeypair.h"

#include <iostream>

InMemoryIdentityKeyStore::InMemoryIdentityKeyStore()
{
}

bool InMemoryIdentityKeyStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    if (trustedKeys.find(recipientId) != trustedKeys.end()) {
        return true;
    }
    return trustedKeys[recipientId] == identityKey;
}
