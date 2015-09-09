#ifndef MESSAGEKEYS_H
#define MESSAGEKEYS_H

#include "byteutil.h"

class MessageKeys
{
public:
    MessageKeys();
    MessageKeys(const ByteArray &cipherKey, const ByteArray &macKey, const ByteArray &iv, uint counter);

    ByteArray getCipherKey() const;
    ByteArray getMacKey() const;
    ByteArray getIv() const;
    uint getCounter() const;

private:
    ByteArray cipherKey;
    ByteArray macKey;
    ByteArray iv;
    uint counter;

};

#endif // MESSAGEKEYS_H
