#include "curve.h"
#include "invalidkeyexception.h"
#include "libcurve25519/curve.h"

#include <string.h>

const int Curve::DJB_TYPE = 5;

ECKeyPair Curve::generateKeyPair()
{
    unsigned char buff1[32];
    for (unsigned i = 0; i < 32; i++)
        buff1[i] = rand();

    Curve25519::generatePrivateKey((char*)buff1);
    ByteArray privateKey((const char*)buff1, 32);

    char pubbuf[32] = {0};
    Curve25519::generatePublicKey(privateKey.c_str(), pubbuf);
    ByteArray publicKey(pubbuf, 32);
    return ECKeyPair(DjbECPublicKey(publicKey), DjbECPrivateKey(privateKey));
}

DjbECPublicKey Curve::decodePoint(const ByteArray &privatePoint, int offset)
{
    uint8_t type = privatePoint[0];

    if (type == Curve::DJB_TYPE) {
        type = privatePoint[offset] & 0xFF;
        if (type != Curve::DJB_TYPE) {
            throw InvalidKeyException("Unknown key type: " + std::to_string((int)type));
        }
        ByteArray keyBytes = privatePoint.substr(offset+1, 32);
        DjbECPublicKey pubkey(keyBytes);
        return pubkey;
    }
    else {
        throw InvalidKeyException("Unknown key type: " + std::to_string((int)type));
    }
}

DjbECPrivateKey Curve::decodePrivatePoint(const ByteArray &privatePoint)
{
    return DjbECPrivateKey(privatePoint);
}

ByteArray Curve::calculateAgreement(const DjbECPublicKey &publicKey, const DjbECPrivateKey &privateKey)
{
    if (publicKey.getType() != privateKey.getType()) {
        throw InvalidKeyException("Public and private keys must be of the same type!");
    }

    if (publicKey.getType() == DJB_TYPE) {
        char buf[32] = {0};
        Curve25519::calculateAgreement(privateKey.getPrivateKey().c_str(),
                                       publicKey.getPublicKey().c_str(),
                                       buf);
        return ByteArray(buf, 32);
    } else {
        throw InvalidKeyException("Unknown type: " + publicKey.getType());
    }
}

bool Curve::verifySignature(const DjbECPublicKey &signingKey, const ByteArray &message, const ByteArray &signature)
{
    if (signingKey.getType() == DJB_TYPE) {
        return Curve25519::verifySignature((const unsigned char*)signingKey.getPublicKey().c_str(),
                                           (const unsigned char*)message.c_str(),
                                           message.size(),
                                           (const unsigned char*)signature.c_str());
    } else {
        throw InvalidKeyException("Unknown type: " + std::to_string((int)signingKey.getType()));
    }
}

ByteArray Curve::calculateSignature(const DjbECPrivateKey &signingKey, const ByteArray &message)
{
    if (signingKey.getType() == DJB_TYPE) {
        unsigned char buff1[64];
        for (unsigned i = 0; i < 64; i++)
            buff1[i] = rand();

        ByteArray random64((const char*)buff1, 64);
        ByteArray signature(64, '\0');
        Curve25519::calculateSignature((const unsigned char*)signingKey.getPrivateKey().c_str(),
                                       (const unsigned char*)message.c_str(),
                                       message.size(),
                                       (const unsigned char*)random64.c_str(),
                                       (unsigned char*)signature.data());
        return signature;
    } else {
        throw InvalidKeyException("Unknown type: " + signingKey.getType());
    }
}
