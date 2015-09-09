#include "signedprekeyrecord.h"
#include "curve.h"

SignedPreKeyRecord::SignedPreKeyRecord(uint64_t id, uint64_t timestamp, const ECKeyPair &keyPair, const ByteArray &signature)
{
    ByteArray bytePublic = keyPair.getPublicKey().serialize();
    ByteArray bytePrivate = keyPair.getPrivateKey().serialize();

    structure.set_id(id);
    structure.set_publickey(bytePublic.c_str(), bytePublic.size());
    structure.set_privatekey(bytePrivate.c_str(), bytePrivate.size());
    structure.set_signature(signature.c_str(), signature.size());
    structure.set_timestamp(timestamp);
}

SignedPreKeyRecord::SignedPreKeyRecord(const ByteArray &serialized)
{
    structure.ParsePartialFromArray(serialized.c_str(), serialized.size());
}

uint64_t SignedPreKeyRecord::getId() const
{
    return structure.id();
}

uint64_t SignedPreKeyRecord::getTimestamp() const
{
    return structure.timestamp();
}

ECKeyPair SignedPreKeyRecord::getKeyPair() const
{
    ::std::string publickey = structure.publickey();
    ByteArray publicPoint(publickey.data(), publickey.length());
    DjbECPublicKey publicKey = Curve::decodePoint(publicPoint, 0);

    ::std::string privatekey = structure.privatekey();
    ByteArray privatePoint(privatekey.data(), privatekey.length());
    DjbECPrivateKey privateKey = Curve::decodePrivatePoint(privatePoint);

    return ECKeyPair(publicKey, privateKey);
}

ByteArray SignedPreKeyRecord::getSignature() const
{
    ::std::string signature = structure.signature();
    return ByteArray(signature.data(), signature.length());
}

ByteArray SignedPreKeyRecord::serialize() const
{
    ::std::string serialized = structure.SerializeAsString();
    return ByteArray(serialized.data(), serialized.length());
}
