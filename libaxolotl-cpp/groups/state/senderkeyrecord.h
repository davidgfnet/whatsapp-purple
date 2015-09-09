#ifndef SENDERKEYRECORD_H
#define SENDERKEYRECORD_H

#include <vector>
#include "byteutil.h"

#include "senderkeystate.h"
#include "djbec.h"
#include "eckeypair.h"

class SenderKeyRecord
{
public:
    SenderKeyRecord();
    SenderKeyRecord(const ByteArray &serialized);
    SenderKeyState *getSenderKeyState(int keyId = 0);
    void addSenderKeyState(int id, int iteration, const ByteArray &chainKey, const DjbECPublicKey &signatureKey);
    void setSenderKeyState(int id, int iteration, const ByteArray &chainKey, const ECKeyPair &signatureKey);
    ByteArray serialize() const;

private:
    std::vector<SenderKeyState*> senderKeyStates;
};

#endif // SENDERKEYRECORD_H
