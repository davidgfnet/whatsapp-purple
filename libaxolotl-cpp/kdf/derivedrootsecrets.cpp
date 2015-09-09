#include "derivedrootsecrets.h"

const int DerivedRootSecrets::SIZE = 64;

DerivedRootSecrets::DerivedRootSecrets(const ByteArray &okm)
{
    std::vector<ByteArray> keys = ByteUtil::split(okm, 32, 32);
    rootKey = keys[0];
    chainKey = keys[1];
}

ByteArray DerivedRootSecrets::getRootKey() const
{
    return rootKey;
}

ByteArray DerivedRootSecrets::getChainKey() const
{
    return chainKey;
}
