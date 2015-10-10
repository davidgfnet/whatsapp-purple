
#include "curve25519test.h"
#include "ratchet/rootkeytest.h"
#include "kdf/hkdftest.h"
#include "ratchet/chainkeytest.h"
#include "ratchet/ratchetingsessiontest.h"
#include "sessionbuildertest.h"
#include "sessionciphertest.h"

int main(int argc, char *argv[])
{
    Curve25519Test curve25519test;
    curve25519test.testCurve();
    curve25519test.testAgreement();
    curve25519test.testRandomAgreements();
    curve25519test.testSignature();

    HKDFTest hkdfTest;
    hkdfTest.testVectorV3();
    hkdfTest.testVectorV2();
    hkdfTest.testVectorLongV3();

    ChainKeyTest chainKeyTest;
    chainKeyTest.testChainKeyDerivationV2();

    RootKeyTest rootKeyTest;
    rootKeyTest.testRootKeyDerivationV2();

    RatchetingSessionTest ratchetingSessionTest;
    ratchetingSessionTest.testRatchetingSessionAsBob();
    ratchetingSessionTest.testRatchetingSessionAsAlice();

    SessionCipherTest sessionCipherTest;
    sessionCipherTest.testBasicSessionV2();
    sessionCipherTest.testBasicSessionV3();

    SessionBuilderTest sessionBuilderTest;
    sessionBuilderTest.testBasicPreKeyV2();

    return 0;
}

