#include "derivedmessagesecrets.h"

#include <vector>

const int DerivedMessageSecrets::SIZE = 80;
const int DerivedMessageSecrets::CIPHER_KEY_LENGTH = 32;
const int DerivedMessageSecrets::MAC_KEY_LENGTH = 32;
const int DerivedMessageSecrets::IV_LENGTH = 16;

DerivedMessageSecrets::DerivedMessageSecrets(const ByteArray &okm)
{
    std::vector<ByteArray> keys = ByteUtil::split(okm,
                                             DerivedMessageSecrets::CIPHER_KEY_LENGTH,
                                             DerivedMessageSecrets::MAC_KEY_LENGTH,
                                             DerivedMessageSecrets::IV_LENGTH);
    cipherKey = keys[0]; //AES
    macKey = keys[1]; //sha256
    iv = keys[2];
}

ByteArray DerivedMessageSecrets::getCipherKey() const
{
    return cipherKey;
}

ByteArray DerivedMessageSecrets::getMacKey() const
{
    return macKey;
}

ByteArray DerivedMessageSecrets::getIv() const
{
    return iv;
}
