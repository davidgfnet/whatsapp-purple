#include "inmemorysenderkeystore.h"

InMemorySenderKeyStore::InMemorySenderKeyStore()
{
}

void InMemorySenderKeyStore::storeSenderKey(const QByteArray &senderKeyId, SenderKeyRecord *record)
{
    store[senderKeyId] = record;
}

SenderKeyRecord InMemorySenderKeyStore::loadSenderKey(const QByteArray &senderKeyId)
{
    if (store.contains(senderKeyId))
        return SenderKeyRecord(store[senderKeyId]->serialize());

    return SenderKeyRecord();
}
