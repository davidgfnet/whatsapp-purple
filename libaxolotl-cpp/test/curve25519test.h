#ifndef CURVE25519TEST_H
#define CURVE25519TEST_H

class Curve25519Test
{
public:
    Curve25519Test();

    void testCurve();
    void simpleTest();
    void testAgreement();
    void testRandomAgreements();
    void testSignature();
};

#endif // CURVE25519TEST_H
