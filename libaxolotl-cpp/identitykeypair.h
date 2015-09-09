#ifndef IDENTITYKEYPAIR_H
#define IDENTITYKEYPAIR_H

#include "djbec.h"
#include "identitykey.h"
#include "byteutil.h"

class IdentityKeyPair
{
public:
    IdentityKeyPair();
    IdentityKeyPair(const IdentityKey &publicKey, const DjbECPrivateKey &privateKey);
    IdentityKeyPair(const ByteArray &serialized);

    IdentityKey getPublicKey() const;
    DjbECPrivateKey getPrivateKey() const;

private:
    IdentityKey publicKey;
    DjbECPrivateKey privateKey;

};

#endif // IDENTITYKEYPAIR_H
