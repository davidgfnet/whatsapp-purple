#ifndef CURVE_DLL_H
#define CURVE_DLL_H

#include "curve_global.h"

class LIBCURVE_DLL Curve25519
{
public:
    Curve25519();

    static void generatePrivateKey(char *random);
    static void generatePublicKey(const char *privatekey, char *mypublic);
    static void calculateAgreement(const char *myprivate, const char *theirpublic, char *shared_key);
    static void calculateSignature(const unsigned char *privatekey, const unsigned char *message, const unsigned long messagelen, const unsigned char *random, unsigned char *signature);
    static int verifySignature(const unsigned char *publickey, const unsigned char *message, const unsigned long messagelen, const unsigned char *signature);
};

#endif  // CURVE_DLL_H
