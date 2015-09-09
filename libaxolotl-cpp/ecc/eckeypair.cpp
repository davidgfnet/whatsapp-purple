#include "eckeypair.h"

ECKeyPair::ECKeyPair()
{

}

ECKeyPair::ECKeyPair(const DjbECPublicKey &publicKey, const DjbECPrivateKey &privateKey)
{
    this->publicKey = publicKey;
    this->privateKey = privateKey;
}

DjbECPrivateKey ECKeyPair::getPrivateKey() const
{
    return privateKey;
}

DjbECPublicKey ECKeyPair::getPublicKey() const
{
    return publicKey;
}
