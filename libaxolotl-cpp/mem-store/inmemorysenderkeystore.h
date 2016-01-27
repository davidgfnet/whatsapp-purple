#ifndef INMEMORYSENDERKEYSTORE_H
#define INMEMORYSENDERKEYSTORE_H

#include <map>
#include "groups/state/senderkeystore.h"
#include "byteutil.h"
#include "serializer.h"

class InMemorySenderKeyStore : public SenderKeyStore
{
public:
    InMemorySenderKeyStore();
	InMemorySenderKeyStore(Unserializer uns);

    void storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record);
    SenderKeyRecord loadSenderKey(const ByteArray &senderKeyId) const;

	std::string serialize() const;

private:
    std::map<ByteArray, SenderKeyRecord> store;
};

#endif // INMEMORYSENDERKEYSTORE_H
