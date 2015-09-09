#include "identitykeypair.h"
#include "curve.h"
#include "sessionstate.h"

IdentityKeyPair::IdentityKeyPair()
{

}

IdentityKeyPair::IdentityKeyPair(const IdentityKey &publicKey, const DjbECPrivateKey &privateKey)
{
    this->publicKey = publicKey;
    this->privateKey = privateKey;
}

IdentityKeyPair::IdentityKeyPair(const ByteArray &serialized)
{
    textsecure::IdentityKeyPairStructure structure;
    structure.ParseFromArray(serialized.c_str(), serialized.size());
    ::std::string publickey = structure.publickey();
    this->publicKey  = IdentityKey(ByteArray(publickey.data(), publickey.length()), 0);
    ::std::string privatekey = structure.privatekey();
    this->privateKey = Curve::decodePrivatePoint(ByteArray(privatekey.data(), privatekey.length()));
}

IdentityKey IdentityKeyPair::getPublicKey() const
{
    return publicKey;
}

DjbECPrivateKey IdentityKeyPair::getPrivateKey() const
{
    return privateKey;
}
