#ifndef DERIVEDROOTSECRETS_H
#define DERIVEDROOTSECRETS_H

#include "byteutil.h"

class DerivedRootSecrets
{
public:
    DerivedRootSecrets(const ByteArray &okm);
    static const int SIZE;

    ByteArray getRootKey() const;
    ByteArray getChainKey() const;

private:
    ByteArray rootKey;
    ByteArray chainKey;

};

#endif // DERIVEDROOTSECRETS_H
