#include "inmemoryidentitykeystore.h"
#include "serializer.h"

#include "ecc/curve.h"
#include "ecc/eckeypair.h"

#include <iostream>
#include <assert.h>

InMemoryIdentityKeyStore::InMemoryIdentityKeyStore(Unserializer &uns)
{
	unsigned int magic = uns.readInt32();
	assert(magic == 0x1D387171);
	unsigned int n = uns.readInt32();
	while (n--) {
		uint64_t key = uns.readInt64();
		IdentityKey val(uns.readString());
		trustedKeys[key] = val;
	}

	localRegistrationId = uns.readInt64();

	IdentityKey publicKey;
	DjbECPrivateKey privateKey;

	std::string pubkey = uns.readString();
	if (pubkey.size())
		publicKey = IdentityKey(pubkey);

	std::string privkey = uns.readString();
	if (privkey.size())
		privateKey = DjbECPrivateKey(privkey);

	identityKeyPair = IdentityKeyPair(publicKey, privateKey);
}

bool InMemoryIdentityKeyStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
	// If identify not in the list return true? WTF!
    if (trustedKeys.find(recipientId) == trustedKeys.end()) {
        return true;
    }
    return trustedKeys[recipientId] == identityKey;
}

void InMemoryIdentityKeyStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey) {
	trustedKeys[recipientId] = identityKey;
}


std::string InMemoryIdentityKeyStore::serialize() const
{
	Serializer ser;
	ser.putInt32(0x1D387171);
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


