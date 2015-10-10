#include "messagekeys.h"

MessageKeys::MessageKeys()
{

}

MessageKeys::MessageKeys(const ByteArray &cipherKey, const ByteArray &macKey, const ByteArray &iv, unsigned int counter)
{
    this->cipherKey = cipherKey;
    this->macKey = macKey;
    this->iv = iv;
    this->counter = counter;
}

ByteArray MessageKeys::getCipherKey() const
{
    return cipherKey;
}

ByteArray MessageKeys::getMacKey() const
{
    return macKey;
}

ByteArray MessageKeys::getIv() const
{
    return iv;
}

unsigned int MessageKeys::getCounter() const
{
    return counter;
}
