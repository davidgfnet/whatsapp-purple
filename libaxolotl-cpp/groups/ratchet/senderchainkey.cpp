#include "senderchainkey.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>

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
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int outlen;
    HMAC(EVP_sha256(), key.c_str(), key.size(), (unsigned char*)seed.c_str(), seed.size(), out, &outlen);
    return ByteArray((const char*)out, outlen);
}
