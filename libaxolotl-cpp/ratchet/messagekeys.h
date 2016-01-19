#ifndef MESSAGEKEYS_H
#define MESSAGEKEYS_H

#include "byteutil.h"

class MessageKeys
{
public:
    MessageKeys();
    MessageKeys(const ByteArray &cipherKey, const ByteArray &macKey, const ByteArray &iv, unsigned counter);

    ByteArray getCipherKey() const;
    ByteArray getMacKey() const;
    ByteArray getIv() const;
    unsigned getCounter() const;

private:
    ByteArray cipherKey;
    ByteArray macKey;
    ByteArray iv;
    unsigned counter;

};

#endif // MESSAGEKEYS_H
