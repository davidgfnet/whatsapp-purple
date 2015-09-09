#ifndef SENDERKEYSTORE_H
#define SENDERKEYSTORE_H

#include "senderkeyrecord.h"
#include "byteutil.h"

class SenderKeyStore
{
public:
    virtual void storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record) = 0;
    virtual SenderKeyRecord loadSenderKey(const ByteArray &senderKeyId) const = 0;
};

#endif // SENDERKEYSTORE_H
