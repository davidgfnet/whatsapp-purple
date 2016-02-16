#include "inmemorysenderkeystore.h"

InMemorySenderKeyStore::InMemorySenderKeyStore()
{
}

InMemorySenderKeyStore::InMemorySenderKeyStore(Unserializer &uns)
{
	unsigned int n = uns.readInt32();
	while (n--) {
		std::string key = uns.readString();
		store[key] = SenderKeyRecord(uns.readString());
	}
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

std::string InMemorySenderKeyStore::serialize() const
{
	Serializer ser;
	ser.putInt32(store.size());

	for (auto & key: store) {
		ser.putString(key.first);
		ser.putString(key.second.serialize());
	}

	return ser.getBuffer();	
}


