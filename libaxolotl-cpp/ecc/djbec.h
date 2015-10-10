#ifndef DJBEC_H
#define DJBEC_H

#include "byteutil.h"

class DjbECPublicKey
{
public:
    DjbECPublicKey();
    DjbECPublicKey(const DjbECPublicKey &publicKey);
    DjbECPublicKey(const ByteArray &publicKey);
    ByteArray serialize() const;
    int getType() const;
    ByteArray getPublicKey() const;
    bool operator <(const DjbECPublicKey &otherKey);
    bool operator ==(const DjbECPublicKey &otherKey);

private:
    ByteArray publicKey;

};

class DjbECPrivateKey
{
public:
    DjbECPrivateKey();
    DjbECPrivateKey(const DjbECPrivateKey &privateKey);
    DjbECPrivateKey(const ByteArray &privateKey);
    ByteArray serialize() const;
    int getType() const;
    ByteArray getPrivateKey() const;
    bool operator <(const DjbECPrivateKey &otherKey);
    bool operator ==(const DjbECPrivateKey &otherKey);

private:
    ByteArray privateKey;

};

#endif // DJBEC_H
