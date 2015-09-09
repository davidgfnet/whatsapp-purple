#ifndef IDENTITYKEY_H
#define IDENTITYKEY_H

#include "djbec.h"
#include "byteutil.h"

class IdentityKey
{
public:
    IdentityKey();
    IdentityKey(const DjbECPublicKey &publicKey, int offset = 0);
    IdentityKey(const ByteArray &publicKey, int offset = 0);

    DjbECPublicKey getPublicKey() const;
    ByteArray serialize() const;
    ByteArray getFingerprint() const;
    ByteArray hashCode() const;
    bool operator ==(const IdentityKey &otherKey);

private:
    DjbECPublicKey publicKey;

};

#endif // IDENTITYKEY_H
