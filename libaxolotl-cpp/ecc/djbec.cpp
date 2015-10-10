#include "djbec.h"
#include "curve.h"

DjbECPublicKey::DjbECPublicKey()
{
    this->publicKey.clear();
}

DjbECPublicKey::DjbECPublicKey(const DjbECPublicKey &publicKey)
{
    this->publicKey = publicKey.getPublicKey();
}

DjbECPublicKey::DjbECPublicKey(const ByteArray &publicKey)
{
    this->publicKey = publicKey;
}

ByteArray DjbECPublicKey::serialize() const
{
    if (!publicKey.empty()) {
        ByteArray serialized(1, (char)Curve::DJB_TYPE);
        serialized += publicKey;
        return serialized;
    }
    return ByteArray();
}

int DjbECPublicKey::getType() const
{
    return Curve::DJB_TYPE;
}

ByteArray DjbECPublicKey::getPublicKey() const
{
    return publicKey;
}

bool DjbECPublicKey::operator <(const DjbECPublicKey &otherKey)
{
    return publicKey != otherKey.publicKey;
}

bool DjbECPublicKey::operator ==(const DjbECPublicKey &otherKey)
{
    return publicKey == otherKey.publicKey;
}

DjbECPrivateKey::DjbECPrivateKey()
{
    this->privateKey.clear();
}

DjbECPrivateKey::DjbECPrivateKey(const DjbECPrivateKey &privateKey)
{
    this->privateKey = privateKey.getPrivateKey();
}

DjbECPrivateKey::DjbECPrivateKey(const ByteArray &privateKey)
{
    this->privateKey = privateKey;
}

ByteArray DjbECPrivateKey::serialize() const
{
    return privateKey;
}

int DjbECPrivateKey::getType() const
{
    return Curve::DJB_TYPE;
}

ByteArray DjbECPrivateKey::getPrivateKey() const
{
    return privateKey;
}

bool DjbECPrivateKey::operator <(const DjbECPrivateKey &otherKey)
{
    return privateKey != otherKey.privateKey;
}

bool DjbECPrivateKey::operator ==(const DjbECPrivateKey &otherKey)
{
    return privateKey == otherKey.privateKey;
}
