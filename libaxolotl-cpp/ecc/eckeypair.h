#ifndef ECKEYPAIR_H
#define ECKEYPAIR_H

#include "djbec.h"

class ECKeyPair
{
public:
    ECKeyPair();
    ECKeyPair(const DjbECPublicKey &publicKey, const DjbECPrivateKey &privateKey);
    DjbECPrivateKey getPrivateKey() const;
    DjbECPublicKey getPublicKey() const;

private:
    DjbECPublicKey publicKey;
    DjbECPrivateKey privateKey;

};

#endif // ECKEYPAIR_H
