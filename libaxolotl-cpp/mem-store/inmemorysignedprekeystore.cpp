#include "inmemorysignedprekeystore.h"
#include "serializer.h"
#include "invalidkeyidexception.h"

#include "byteutil.h"
#include <vector>

InMemorySignedPreKeyStore::InMemorySignedPreKeyStore(Unserializer uns)
{
	unsigned int n = uns.readInt32();
	while (n--) {
		uint64_t key = uns.readInt64();
		store[key] = uns.readString();
	}
}

SignedPreKeyRecord InMemorySignedPreKeyStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
	if (store.find(signedPreKeyId) != store.end()) {
		return SignedPreKeyRecord(store.at(signedPreKeyId));
	}
	throw WhisperException("No such signedprekeyrecord! " + std::to_string(signedPreKeyId));
}

std::vector<SignedPreKeyRecord> InMemorySignedPreKeyStore::loadSignedPreKeys()
{
	std::vector<SignedPreKeyRecord> results;
	for (auto & key: store) {
		results.push_back(SignedPreKeyRecord(key.second));
	}
	return results;
}

void InMemorySignedPreKeyStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
	store[signedPreKeyId] = record.serialize();
}

bool InMemorySignedPreKeyStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
	return store.find(signedPreKeyId) != store.end();
}

void InMemorySignedPreKeyStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
	store.erase(signedPreKeyId);
}

std::string InMemorySignedPreKeyStore::serialize() const
{
	Serializer ser;
	ser.putInt32(store.size());

	for (auto & key: store) {
		ser.putInt64(key.first);
		ser.putString(key.second);
	}

	return ser.getBuffer();	
}


