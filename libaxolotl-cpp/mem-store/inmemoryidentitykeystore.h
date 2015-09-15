#ifndef INMEMORYIDENTITYKEYSTORE_H
#define INMEMORYIDENTITYKEYSTORE_H

#include "state/identitykeystore.h"
#include "identitykeypair.h"
#include "serializer.h"
#include "util/keyhelper.h"

#include <map>

class InMemoryIdentityKeyStore : public IdentityKeyStore
{
public:
	InMemoryIdentityKeyStore() {}
	InMemoryIdentityKeyStore(Unserializer uns);

    IdentityKeyPair getIdentityKeyPair() { return identityKeyPair; }

	unsigned int getLocalRegistrationId() {
		return localRegistrationId;
	}

	void saveIdentity(uint64_t recipientId, const IdentityKey &identityKey) {
		trustedKeys[recipientId] = identityKey;
	}

	void storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair) {
		this->localRegistrationId = registrationId;
		this->identityKeyPair = identityKeyPair;
	}

	void removeIdentity(uint64_t recipientId) {
		trustedKeys.erase(recipientId);
	}

	bool isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey);
	std::string serialize() const;

private:
	std::map<uint64_t, IdentityKey> trustedKeys;
	uint64_t localRegistrationId;

	// Out own identity
	IdentityKeyPair identityKeyPair;
};

#endif // INMEMORYIDENTITYKEYSTORE_H
