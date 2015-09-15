#include "inmemoryidentitykeystore.h"
#include "serializer.h"

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

std::string InMemoryIdentityKeyStore::serialize() const
{
	Serializer ser;
	ser.putInt32(trustedKeys.size());

	for (auto & key: trustedKeys) {
		ser.putInt64(key.first);
		ser.putString(key.second.serialize());
	}

	ser.putInt64(localRegistrationId);
	ser.putString(identityKeyPair.getPublicKey().serialize());
	ser.putString(identityKeyPair.getPrivateKey().serialize());

	return ser.getBuffer();	
}


