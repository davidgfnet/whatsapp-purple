#ifndef DERIVEDMESSAGESECRETS_H
#define DERIVEDMESSAGESECRETS_H

#include "byteutil.h"

class DerivedMessageSecrets
{
public:
    DerivedMessageSecrets(const ByteArray &okm);
    static const int SIZE;
    static const int CIPHER_KEY_LENGTH;
    static const int MAC_KEY_LENGTH;
    static const int IV_LENGTH;

    ByteArray getCipherKey() const;
    ByteArray getMacKey() const;
    ByteArray getIv() const;

private:
    ByteArray cipherKey;
    ByteArray macKey;
    ByteArray iv;

};

#endif // DERIVEDMESSAGESECRETS_H
