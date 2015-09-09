#include "identitykey.h"
#include "curve.h"

IdentityKey::IdentityKey()
{
}

IdentityKey::IdentityKey(const DjbECPublicKey &publicKey, int offset)
{
    if (offset == 0) {
        this->publicKey = publicKey;
    }
    else {
        this->publicKey = Curve::decodePoint(publicKey.serialize(), offset);
    }
}

IdentityKey::IdentityKey(const ByteArray &publicKey, int offset)
{
    this->publicKey = Curve::decodePoint(publicKey, offset);
}

DjbECPublicKey IdentityKey::getPublicKey() const
{
    return publicKey;
}

ByteArray IdentityKey::serialize() const
{
    return publicKey.serialize();
}

ByteArray IdentityKey::getFingerprint() const
{
    return ByteUtil::toHex(publicKey.serialize());
}

ByteArray IdentityKey::hashCode() const
{
    return publicKey.serialize().substr(0, 4); // TODO
}

bool IdentityKey::operator ==(const IdentityKey &otherKey)
{
    return publicKey.serialize() == otherKey.getPublicKey().serialize();
}
