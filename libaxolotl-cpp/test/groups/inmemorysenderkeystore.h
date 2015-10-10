#ifndef INMEMORYSENDERKEYSTORE_H
#define INMEMORYSENDERKEYSTORE_H

#include "../libaxolotl/groups/state/senderkeystore.h"

#include <QByteArray>
#include <QHash>

class InMemorySenderKeyStore : public SenderKeyStore
{
public:
    InMemorySenderKeyStore();

    void storeSenderKey(const QByteArray &senderKeyId, SenderKeyRecord *record);
    SenderKeyRecord *loadSenderKey(const QByteArray &senderKeyId);

private:
    QHash<QByteArray, SenderKeyRecord*> store;
};

#endif // INMEMORYSENDERKEYSTORE_H
