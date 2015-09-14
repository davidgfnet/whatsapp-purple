#include "curve25519test.h"

#include "../libcurve25519/curve.h"
#include "../libcurve25519/curve_global.h"

#include "ecc/curve.h"
#include "ecc/djbec.h"
#include "util/byteutil.h"

#include "invalidkeyexception.h"

#include <openssl/rand.h>
#include <string.h>
#include <byteutil.h>

#include <iostream>

Curve25519Test::Curve25519Test()
{
}

void Curve25519Test::testCurve()
{
    std::cerr << "testCurve" << std::endl;

    RAND_poll();

    unsigned char buff1[32];
    memset(buff1, 0, 32);
    RAND_bytes(buff1, 32);

    unsigned char buff2[64];
    memset(buff2, 0, 64);
    RAND_bytes(buff2, 64);

    ByteArray rand2 = ByteArray((const char*)buff2, 64);

    Curve25519::generatePrivateKey((char*)buff1);
    ByteArray rand1 = ByteArray((const char*)buff1, 32);
    ByteArray privateKey = rand1;

	char tmp[32] = {0};

    Curve25519::generatePublicKey(privateKey.c_str(), tmp);
    ByteArray publicKey((const char*)tmp, 32);
    ByteArray message = publicKey;

	char tmp2[32] = {0};
    Curve25519::calculateAgreement(privateKey.c_str(), publicKey.c_str(), tmp2);
    ByteArray agreement((const char*)tmp2, 32);

    ByteArray signature(64, '\0');
    Curve25519::calculateSignature((unsigned char*)privateKey.c_str(),
                                   (unsigned char*)message.c_str(), message.size(),
                                   (unsigned char*)rand2.c_str(),
                                   (unsigned char*)signature.data());
    int verified = Curve25519::verifySignature((unsigned char*)publicKey.c_str(),
                                               (unsigned char*)message.c_str(), message.size(),
                                               (unsigned char*)signature.c_str());

    std::cerr << "VERIFIED " << (verified == 0) << std::endl;

    if (verified != 0) {
        std::cerr << "RANDM32  " << rand1.size() << ByteUtil::toHex(rand1) << std::endl;
        std::cerr << "RANDM64  " << rand2.size() << ByteUtil::toHex(rand2) << std::endl;
        std::cerr << "PRIVATE  " << privateKey.size() << ByteUtil::toHex(privateKey) << std::endl;
        std::cerr << "PUBLIC   " << publicKey.size() << ByteUtil::toHex(publicKey) << std::endl;
        std::cerr << "MESSAGE  " << message.size() << ByteUtil::toHex(message) << std::endl;
        std::cerr << "AGREEMENT" << agreement.size() << ByteUtil::toHex(agreement) << std::endl;
        std::cerr << "SIGNATURE" << signature.size() << ByteUtil::toHex(signature) << std::endl;
    }
}

void Curve25519Test::simpleTest()
{
}

void Curve25519Test::testAgreement()
{
    std::cerr << "testAgreement" << std::endl;

    try {
        ByteArray alicePublic  = ByteUtil::fromHex("051bb75966f2e93a3691dfff942bb2a466a1c08b8d78ca3f4d6df8b8bfa2e4ee28");
        ByteArray alicePrivate = ByteUtil::fromHex("c806439dc9d2c476ffed8f2580c0888d58ab406bf7ae3698879021b96bb4bf59");
        ByteArray bobPublic    = ByteUtil::fromHex("05653614993d2b15ee9e5fd3d86ce719ef4ec1daae1886a87b3f5fa9565a27a22f");
        ByteArray bobPrivate   = ByteUtil::fromHex("b03b34c33a1c44f225b662d2bf4859b8135411fa7b0386d45fb75dc5b91b4466");
        ByteArray shared       = ByteUtil::fromHex("325f239328941ced6e673b86ba41017448e99b649a9c3806c1dd7ca4c477e629");

        DjbECPublicKey  alicePublicKey = Curve::decodePoint(alicePublic, 0);
        DjbECPrivateKey alicePrivateKey = Curve::decodePrivatePoint(alicePrivate);

        DjbECPublicKey  bobPublicKey = Curve::decodePoint(bobPublic, 0);
        DjbECPrivateKey bobPrivateKey = Curve::decodePrivatePoint(bobPrivate);

        ByteArray sharedOne = Curve::calculateAgreement(alicePublicKey, bobPrivateKey);
        ByteArray sharedTwo = Curve::calculateAgreement(bobPublicKey, alicePrivateKey);

        bool status1 = (sharedOne == shared);
        bool status2 = (sharedTwo == shared);

        std::cerr << "VERIFIED " << (status1 && status2) << std::endl;

        if (!status1 || !status2) {
            std::cerr << "alicePublicKey: " << ByteUtil::toHex(alicePublicKey.serialize()) << std::endl;
            std::cerr << "alicePrivateKey:" << ByteUtil::toHex(alicePrivateKey.serialize()) << std::endl;
            std::cerr << "bobPublicKey:   " << ByteUtil::toHex(bobPublicKey.serialize()) << std::endl;
            std::cerr << "bobPrivateKey:  " << ByteUtil::toHex(bobPrivateKey.serialize()) << std::endl;
            std::cerr << "shared   :      " << ByteUtil::toHex(shared) << std::endl;
            std::cerr << "sharedOne:      " << ByteUtil::toHex(sharedOne) << std::endl;
            std::cerr << "sharedTwo:      " << ByteUtil::toHex(sharedTwo) << std::endl;
        }
    }
    catch (InvalidKeyException &e) {
        std::cerr << "InvalidKeyException" << e.errorMessage() << std::endl;
    }
}

void Curve25519Test::testRandomAgreements()
{
    std::cerr << "testRandomAgreements" << std::endl;

    int i;
    for (i = 0; i < 50; i++) {
        ECKeyPair alice = Curve::generateKeyPair();
        ECKeyPair bob   = Curve::generateKeyPair();
        ByteArray sharedAlice = Curve::calculateAgreement(bob.getPublicKey(),   alice.getPrivateKey());
        ByteArray sharedBob   = Curve::calculateAgreement(alice.getPublicKey(), bob.getPrivateKey());

        if (sharedAlice != sharedBob) {
            std::cerr << "FAILED at " << i << std::endl;
            break;
        }
    }
    if (i == 50) {
        std::cerr << "VERIFIED" << std::endl;
    }
}

void Curve25519Test::testSignature()
{
    std::cerr << "testSignature" << std::endl;

    ByteArray aliceIdentityPrivate = ByteUtil::fromHex("c097248412e58bf05df487968205132794178e367637f5818f81e0e6ce73e865");
    ByteArray aliceIdentityPublic  = ByteUtil::fromHex("05ab7e717d4a163b7d9a1d8071dfe9dcf8cdcd1cea3339b6356be84d887e322c64");
    ByteArray aliceEphemeralPublic = ByteUtil::fromHex("05edce9d9c415ca78cb7252e72c2c4a554d3eb29485a0e1d503118d1a82d99fb4a");
    ByteArray aliceSignature       = ByteUtil::fromHex("5de88ca9a89b4a115da79109c67c9c7464a3e4180274f1cb8c63c2984e286dfbede82deb9dcd9fae0bfbb821569b3d9001bd8130cd11d486cef047bd60b86e88");

    DjbECPrivateKey alicePrivateKey = Curve::decodePrivatePoint(aliceIdentityPrivate);
    DjbECPublicKey  alicePublicKey  = Curve::decodePoint(aliceIdentityPublic, 0);
    DjbECPublicKey  aliceEphemeral  = Curve::decodePoint(aliceEphemeralPublic, 0);

    int res = Curve::verifySignature(alicePublicKey, aliceEphemeral.serialize(), aliceSignature);

    std::cerr << "VERIFIED " << (res == 0) << std::endl;

    if (res != 0) {
        std::cerr << "alicePrivateKey:" << ByteUtil::toHex(alicePrivateKey.serialize()) << std::endl;
        std::cerr << "alicePublicKey: " << ByteUtil::toHex(alicePublicKey.serialize()) << std::endl;
        std::cerr << "aliceEphemeral: " << ByteUtil::toHex(aliceEphemeral.serialize()) << std::endl;
    }
}




