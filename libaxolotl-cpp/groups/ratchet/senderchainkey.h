#ifndef SENDERCHAINKEY_H
#define SENDERCHAINKEY_H

#include "byteutil.h"
#include "sendermessagekey.h"

class SenderChainKey
{
public:
    SenderChainKey(int iteration, const ByteArray &chainKey);

    int getIteration() const;
    SenderMessageKey getSenderMessageKey() const;
    SenderChainKey getNext() const;
    ByteArray getSeed() const;
    ByteArray getDerivative(const ByteArray &seed, const ByteArray &key) const;

private:
    static const ByteArray MESSAGE_KEY_SEED;
    static const ByteArray CHAIN_KEY_SEED;

    int        iteration;
    ByteArray chainKey;
};

#endif // SENDERCHAINKEY_H
