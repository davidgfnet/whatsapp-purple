#include "inmemorysenderkeystore.h"

InMemorySenderKeyStore::InMemorySenderKeyStore()
{
}

void InMemorySenderKeyStore::storeSenderKey(const QByteArray &senderKeyId, SenderKeyRecord *record)
{
    store[senderKeyId] = record;
}

SenderKeyRecord *InMemorySenderKeyStore::loadSenderKey(const QByteArray &senderKeyId)
{
    if (store.contains(senderKeyId))
        return new SenderKeyRecord(store[senderKeyId]->serialize());

    return new SenderKeyRecord();
}
