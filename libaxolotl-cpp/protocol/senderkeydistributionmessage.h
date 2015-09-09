#ifndef SENDERKEYDISTRIBUTIONMESSAGE_H
#define SENDERKEYDISTRIBUTIONMESSAGE_H

#include "ciphertextmessage.h"
#include "djbec.h"
#include "byteutil.h"

class SenderKeyDistributionMessage : public CiphertextMessage
{
public:
    SenderKeyDistributionMessage(int id, int iteration, const ByteArray &chainKey, const DjbECPublicKey &signatureKey);
    virtual ~SenderKeyDistributionMessage() {}

    ByteArray serialize() const;
    int getType() const;
    int getIteration() const;
    ByteArray getChainKey() const;
    DjbECPublicKey getSignatureKey() const;
    int getId() const;

private:
    int         id;
    int         iteration;
    ByteArray  chainKey;
    DjbECPublicKey signatureKey;
    ByteArray  serialized;
};

#endif // SENDERKEYDISTRIBUTIONMESSAGE_H
