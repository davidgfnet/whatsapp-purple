#include "inmemoryprekeystore.h"
#include "serializer.h"
#include "whisperexception.h"

InMemoryPreKeyStore::InMemoryPreKeyStore()
{
}

PreKeyRecord InMemoryPreKeyStore::loadPreKey(uint64_t preKeyId)
{
    if (store.find(preKeyId) == store.end()) {
        throw WhisperException("No such prekeyRecord!");
    }
    return PreKeyRecord(store.at(preKeyId));
}

void InMemoryPreKeyStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
    store[preKeyId] = record.serialize();
}

bool InMemoryPreKeyStore::containsPreKey(uint64_t preKeyId)
{
    return store.find(preKeyId) != store.end();
}

void InMemoryPreKeyStore::removePreKey(uint64_t preKeyId)
{
    store.erase(preKeyId);
}

std::string InMemoryPreKeyStore::serialize() const
{
	Serializer ser;
	ser.putInt32(store.size());

	for (auto & key: store) {
		ser.putInt64(key.first);
		ser.putString(key.second);
	}

	return ser.getBuffer();	
}

