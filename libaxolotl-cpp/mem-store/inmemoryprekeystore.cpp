#include "inmemoryprekeystore.h"

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
