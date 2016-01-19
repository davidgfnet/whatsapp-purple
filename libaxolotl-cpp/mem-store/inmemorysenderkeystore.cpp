#include "inmemorysenderkeystore.h"

InMemorySenderKeyStore::InMemorySenderKeyStore()
{
}

void InMemorySenderKeyStore::storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record)
{
    store[senderKeyId] = *record;
}

SenderKeyRecord InMemorySenderKeyStore::loadSenderKey(const ByteArray &senderKeyId) const
{
    if (store.find(senderKeyId) != store.end())
        return store.at(senderKeyId);

    return SenderKeyRecord();
}
