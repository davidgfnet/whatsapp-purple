#ifndef INMEMORYSENDERKEYSTORE_H
#define INMEMORYSENDERKEYSTORE_H

#include <map>
#include "groups/state/senderkeystore.h"
#include "byteutil.h"

class InMemorySenderKeyStore : public SenderKeyStore
{
public:
    InMemorySenderKeyStore();

    void storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record);
    SenderKeyRecord loadSenderKey(const ByteArray &senderKeyId) const;
private:
    std::map<ByteArray, SenderKeyRecord> store;
};

#endif // INMEMORYSENDERKEYSTORE_H
