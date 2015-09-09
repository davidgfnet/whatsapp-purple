#ifndef SENDERKEYMESSAGE_H
#define SENDERKEYMESSAGE_H

#include "ciphertextmessage.h"
#include "djbec.h"
#include "byteutil.h"

class SenderKeyMessage : public CiphertextMessage
{
public:
    SenderKeyMessage(const ByteArray &serialized);
    SenderKeyMessage(uint64_t keyId, int iteration, const ByteArray &ciphertext, const DjbECPrivateKey &signatureKey);
    virtual ~SenderKeyMessage() {}

    void verifySignature(const DjbECPublicKey &signatureKey);

    uint64_t getKeyId() const;
    int getIteration() const;
    ByteArray getCipherText() const;
    ByteArray serialize() const;
    int getType() const;

private:
    static const int SIGNATURE_LENGTH = 64;

    ByteArray getSignature(const DjbECPrivateKey &signatureKey, const ByteArray &serialized);

    int         messageVersion;
    uint64_t       keyId;
    int         iteration;
    ByteArray  ciphertext;
    ByteArray  serialized;
};

#endif // SENDERKEYMESSAGE_H
