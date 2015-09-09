#include <stdio.h>
#include <stdint.h>

#include "curve.h"

extern "C" {
#include "src/curve25519-donna.h"
#include "src/ed25519/additions/curve_sigs.h"
}

Curve25519::Curve25519()
{
    printf("No initialization for Curve class. Use static functions.\n");
}

void Curve25519::generatePrivateKey(char *random)
{
    random[0] &= 248;
    random[31] &= 127;
    random[31] |= 64;
}

void Curve25519::generatePublicKey(const char *privatekey, char *mypublic)
{
    //uint8_t mypublic[32];
    const uint8_t basepoint[32] = {9};
    curve25519_donna((uint8_t *)mypublic, (const uint8_t *)privatekey, basepoint);
}

void Curve25519::calculateAgreement(const char *myprivate, const char *theirpublic, char *shared_key)
{
    //uint8_t shared_key[32];
    curve25519_donna((uint8_t *)shared_key, (const uint8_t *)myprivate, (const uint8_t *)theirpublic);
}

int Curve25519::verifySignature(const unsigned char *publickey, const unsigned char *message, const unsigned long messagelen, const unsigned char *signature)
{
    return curve25519_verify(signature, publickey, message, messagelen);
}

void Curve25519::calculateSignature(const unsigned char *privatekey, const unsigned char *message, const unsigned long messagelen, const unsigned char *random, unsigned char *signature)
{
    curve25519_sign(signature, privatekey, message, messagelen, random);
}

