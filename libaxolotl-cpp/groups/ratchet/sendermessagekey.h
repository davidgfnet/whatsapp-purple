#ifndef SENDERMESSAGEKEY_H
#define SENDERMESSAGEKEY_H

#include "byteutil.h"

class SenderMessageKey
{
public:
    SenderMessageKey();
    SenderMessageKey(int iteration, const ByteArray &seed);

    int getIteration() const;
    ByteArray getIv() const;
    ByteArray getCipherKey() const;
    ByteArray getSeed() const;

private:
    int        iteration;
    ByteArray iv;
    ByteArray cipherKey;
    ByteArray seed;
};

#endif // SENDERMESSAGEKEY_H
