#include "senderchainkey.h"

void HMAC_SHA256(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest);

const ByteArray SenderChainKey::MESSAGE_KEY_SEED = ByteArray("\0x01");
const ByteArray SenderChainKey::CHAIN_KEY_SEED = ByteArray("\0x02");

SenderChainKey::SenderChainKey(int iteration, const ByteArray &chainKey)
{
    this->iteration = iteration;
    this->chainKey = chainKey;
}

int SenderChainKey::getIteration() const
{
    return iteration;
}

SenderMessageKey SenderChainKey::getSenderMessageKey() const
{
    return SenderMessageKey(iteration, getDerivative(MESSAGE_KEY_SEED, chainKey));
}

SenderChainKey SenderChainKey::getNext() const
{
    return SenderChainKey(iteration + 1, getDerivative(CHAIN_KEY_SEED, chainKey));
}

ByteArray SenderChainKey::getSeed() const
{
    return chainKey;
}

ByteArray SenderChainKey::getDerivative(const ByteArray &seed, const ByteArray &key) const
{
    unsigned char out[32];
	HMAC_SHA256((unsigned char*)seed.c_str(), seed.size(), (unsigned char*)key.c_str(), key.size(), out);
    return ByteArray((const char*)out, 32);
}
