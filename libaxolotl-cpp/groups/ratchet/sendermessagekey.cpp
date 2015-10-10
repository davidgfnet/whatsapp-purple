#include "sendermessagekey.h"
#include "hkdf.h"
#include <vector>

SenderMessageKey::SenderMessageKey()
{

}

SenderMessageKey::SenderMessageKey(int iteration, const ByteArray &seed)
{
    ByteArray derivative   = HKDF(3).deriveSecrets(seed, ByteArray("WhisperGroup"), 48);
    std::vector<ByteArray> parts = ByteUtil::split(derivative, 16, 32);

    this->iteration = iteration;
    this->seed      = seed;
    this->iv        = parts[0];
    this->cipherKey = parts[1];
}

int SenderMessageKey::getIteration() const
{
    return iteration;
}

ByteArray SenderMessageKey::getIv() const
{
    return iv;
}

ByteArray SenderMessageKey::getCipherKey() const
{
    return cipherKey;
}

ByteArray SenderMessageKey::getSeed() const
{
    return seed;
}
