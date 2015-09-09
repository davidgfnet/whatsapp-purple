#include "prekeyrecord.h"

PreKeyRecord::PreKeyRecord(uint64_t id, const ECKeyPair &keyPair)
{
    ByteArray bytePublic = keyPair.getPublicKey().serialize();
    ByteArray bytePrivate = keyPair.getPrivateKey().serialize();

    structure.set_id(id);
    structure.set_publickey(bytePublic.c_str(), bytePublic.size());
    structure.set_privatekey(bytePrivate.c_str(), bytePrivate.size());
}

PreKeyRecord::PreKeyRecord(const ByteArray &serialized)
{
    structure.ParsePartialFromArray(serialized.c_str(), serialized.size());
}

uint64_t PreKeyRecord::getId() const
{
    return structure.id();
}

ECKeyPair PreKeyRecord::getKeyPair() const
{
    ::std::string publickey = structure.publickey();
    DjbECPublicKey publicKey = Curve::decodePoint(ByteArray(publickey.data(), publickey.length()), 0);
    ::std::string privatekey = structure.privatekey();
    DjbECPrivateKey privateKey = Curve::decodePrivatePoint(ByteArray(privatekey.data(), privatekey.length()));
    return ECKeyPair(publicKey, privateKey);
}

ByteArray PreKeyRecord::serialize() const
{
    std::string str = structure.SerializeAsString();
    ByteArray bytes(str.data(), str.size());
    return bytes;
}
