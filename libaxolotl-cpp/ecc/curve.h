#ifndef CURVE_H
#define CURVE_H

#include "byteutil.h"
#include "eckeypair.h"
#include "djbec.h"

class Curve
{
public:
    static const int DJB_TYPE;

    static ECKeyPair generateKeyPair();
    static DjbECPublicKey decodePoint(const ByteArray &privatePoint, int offset = 0);
    static DjbECPrivateKey decodePrivatePoint(const ByteArray &privatePoint);
    static ByteArray calculateAgreement(const DjbECPublicKey &publicKey, const DjbECPrivateKey &privateKey);
    static bool verifySignature(const DjbECPublicKey &signingKey, const ByteArray &message, const ByteArray &signature);
    static ByteArray calculateSignature(const DjbECPrivateKey &signingKey, const ByteArray &message);
};

#endif // CURVE_H
