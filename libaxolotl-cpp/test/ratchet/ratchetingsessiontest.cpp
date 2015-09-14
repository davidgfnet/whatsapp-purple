#include "ratchetingsessiontest.h"

#include "byteutil.h"
#include <iostream>

#include "identitykey.h"
#include "ecc/curve.h"
#include "ecc/djbec.h"
#include "ecc/eckeypair.h"
#include "ratchet/bobaxolotlparameters.h"
#include "ratchet/aliceaxolotlparameters.h"
#include "ratchet/ratchetingsession.h"
#include "state/sessionstate.h"

RatchetingSessionTest::RatchetingSessionTest()
{
}

void RatchetingSessionTest::testRatchetingSessionAsBob()
{
    std::cerr << "testRatchetingSessionAsBob" << std::endl;

    ByteArray bobPublic            = ByteUtil::fromHex("052cb49776b8770205745a3a6e24f579cdb4ba7a89041005928ebbadc9c05ad458");
    ByteArray bobPrivate           = ByteUtil::fromHex("a1cab48f7c893fafa9880a28c3b4999d28d6329562d27a4ea4e22e9ff1bdd65a");
    ByteArray bobIdentityPublic    = ByteUtil::fromHex("05f1f43874f6966956c2dd473f8fa15adeb71d1cb991b2341692324cefb1c5e626");
    ByteArray bobIdentityPrivate   = ByteUtil::fromHex("4875cc69ddf8ea0719ec947d61081135868d5fd801f02c0225e516df2156605e");
    ByteArray aliceBasePublic      = ByteUtil::fromHex("05472d1fb1a9862c3af6beaca8920277e2b26f4a79213ec7c906aeb35e03cf8950");
    ByteArray aliceEphemeralPublic = ByteUtil::fromHex("056c3e0d1f520283efcc55fca5e67075b904007f1881d151af76df18c51d29d34b");
    ByteArray aliceIdentityPublic  = ByteUtil::fromHex("05b4a8455660ada65b401007f615e654041746432e3339c6875149bceefcb42b4a");
    ByteArray senderChain          = ByteUtil::fromHex("d22fd56d3fec819cf4c3d50c56edfb1c280a1b31964537f1d161e1c93148e36b");

    IdentityKey bobIdentityKeyPublic(bobIdentityPublic, 0);
    DjbECPrivateKey bobIdentityKeyPrivate  = Curve::decodePrivatePoint(bobIdentityPrivate);
    IdentityKeyPair bobIdentityKey(bobIdentityKeyPublic, bobIdentityKeyPrivate);
    DjbECPublicKey bobEphemeralPublicKey   = Curve::decodePoint(bobPublic, 0);
    DjbECPrivateKey bobEphemeralPrivateKey = Curve::decodePrivatePoint(bobPrivate);
    ECKeyPair bobEphemeralKey(bobEphemeralPublicKey, bobEphemeralPrivateKey);
    ECKeyPair bobBaseKey = bobEphemeralKey;

    DjbECPublicKey aliceBasePublicKey       = Curve::decodePoint(aliceBasePublic, 0);
    DjbECPublicKey aliceEphemeralPublicKey  = Curve::decodePoint(aliceEphemeralPublic, 0);
    IdentityKey aliceIdentityPublicKey(aliceIdentityPublic, 0);

    BobAxolotlParameters parameters;
    parameters.setOurIdentityKey(bobIdentityKey);
    parameters.setOurSignedPreKey(bobBaseKey);
    parameters.setOurRatchetKey(bobEphemeralKey);
    //parameters.setOurOneTimePreKey(None);
    parameters.setTheirIdentityKey(aliceIdentityPublicKey);
    parameters.setTheirBaseKey(aliceBasePublicKey);

    SessionState session;

    RatchetingSession::initializeSession(&session, 2, parameters);

    IdentityKey  localIdentityKey = session.getLocalIdentityKey();
    IdentityKey remoteIdentityKey = session.getRemoteIdentityKey();
    ByteArray     senderChainKey = session.getSenderChainKey().getKey();

    bool verified = localIdentityKey == bobIdentityKey.getPublicKey()
            && remoteIdentityKey == aliceIdentityPublicKey
            && senderChainKey == senderChain;

    std::cerr << "VERIFIED " << verified << std::endl;

    if (!verified) {
        std::cerr << "bobIdentityKeyPublic:   " << ByteUtil::toHex(bobIdentityKeyPublic.serialize()) << std::endl;
        std::cerr << "bobIdentityKeyPrivate:  " << ByteUtil::toHex(bobIdentityKeyPrivate.serialize()) << std::endl;
        std::cerr << "bobEphemeralPublicKey:  " << ByteUtil::toHex(bobEphemeralPublicKey.serialize()) << std::endl;
        std::cerr << "bobEphemeralPrivateKey: " << ByteUtil::toHex(bobEphemeralPrivateKey.serialize()) << std::endl;
        std::cerr << "aliceBasePublicKey:     " << ByteUtil::toHex(aliceBasePublicKey.serialize()) << std::endl;
        std::cerr << "aliceEphemeralPublicKey:" << ByteUtil::toHex(aliceEphemeralPublicKey.serialize()) << std::endl;
        std::cerr << "aliceIdentityPublicKey: " << ByteUtil::toHex(aliceIdentityPublicKey.serialize()) << std::endl;
        std::cerr << "localIdentityKey:       " << ByteUtil::toHex(localIdentityKey.serialize()) << std::endl;
        std::cerr << "remoteIdentityKey:      " << ByteUtil::toHex(remoteIdentityKey.serialize()) << std::endl;
        std::cerr << "senderChainKey:         " << ByteUtil::toHex(senderChainKey) << std::endl;
    }
}

void RatchetingSessionTest::testRatchetingSessionAsAlice()
{
    std::cerr << "testRatchetingSessionAsAlice" << std::endl;

    ByteArray bobPublic             = ByteUtil::fromHex("052cb49776b8770205745a3a6e24f579cdb4ba7a89041005928ebbadc9c05ad458");
    ByteArray bobIdentityPublic     = ByteUtil::fromHex("05f1f43874f6966956c2dd473f8fa15adeb71d1cb991b2341692324cefb1c5e626");
    ByteArray aliceBasePublic       = ByteUtil::fromHex("05472d1fb1a9862c3af6beaca8920277e2b26f4a79213ec7c906aeb35e03cf8950");
    ByteArray aliceBasePrivate      = ByteUtil::fromHex("11ae7c64d1e61cd596b76a0db5012673391cae66edbfcf073b4da80516a47449");
    ByteArray aliceEphemeralPublic  = ByteUtil::fromHex("056c3e0d1f520283efcc55fca5e67075b904007f1881d151af76df18c51d29d34b");
    ByteArray aliceEphemeralPrivate = ByteUtil::fromHex("d1ba38cea91743d33939c33c84986509280161b8b60fc7870c599c1d46201248");
    ByteArray aliceIdentityPublic   = ByteUtil::fromHex("05b4a8455660ada65b401007f615e654041746432e3339c6875149bceefcb42b4a");
    ByteArray aliceIdentityPrivate  = ByteUtil::fromHex("9040f0d4e09cf38f6dc7c13779c908c015a1da4fa78737a080eb0a6f4f5f8f58");
    ByteArray receiverChain         = ByteUtil::fromHex("d22fd56d3fec819cf4c3d50c56edfb1c280a1b31964537f1d161e1c93148e36b");

    IdentityKey bobIdentityKey(bobIdentityPublic, 0);
    DjbECPublicKey bobEphemeralPublicKey   = Curve::decodePoint(bobPublic, 0);
    DjbECPublicKey bobBasePublicKey = bobEphemeralPublicKey;

    DjbECPublicKey  aliceBasePublicKey   = Curve::decodePoint(aliceBasePublic, 0);
    DjbECPrivateKey aliceBasePrivateKey  = Curve::decodePrivatePoint(aliceBasePrivate);
    ECKeyPair aliceBaseKey(aliceBasePublicKey, aliceBasePrivateKey);

    DjbECPublicKey  aliceEphemeralPublicKey  = Curve::decodePoint(aliceEphemeralPublic, 0);
    DjbECPrivateKey aliceEphemeralPrivateKey = Curve::decodePrivatePoint(aliceEphemeralPrivate);
    ECKeyPair       aliceEphemeralKey(aliceEphemeralPublicKey, aliceEphemeralPrivateKey);
    IdentityKey     aliceIdentityPublicKey(aliceIdentityPublic, 0);
    DjbECPrivateKey aliceIdentityPrivateKey  = Curve::decodePrivatePoint(aliceIdentityPrivate);
    IdentityKeyPair aliceIdentityKey(aliceIdentityPublicKey, aliceIdentityPrivateKey);

    SessionState session;

    AliceAxolotlParameters parameters;
    parameters.setOurBaseKey(aliceBaseKey);
    parameters.setOurIdentityKey(aliceIdentityKey);
    parameters.setTheirIdentityKey(bobIdentityKey);
    parameters.setTheirSignedPreKey(bobBasePublicKey);
    parameters.setTheirRatchetKey(bobEphemeralPublicKey);

    RatchetingSession::initializeSession(&session, 2, parameters);

    IdentityKey  localIdentityKey = session.getLocalIdentityKey();
    IdentityKey remoteIdentityKey = session.getRemoteIdentityKey();
    ByteArray   receiverChainKey = session.getReceiverChainKey(bobEphemeralPublicKey).getKey();

    bool verified = localIdentityKey == aliceIdentityKey.getPublicKey()
            && remoteIdentityKey == bobIdentityKey
            && receiverChainKey == receiverChain;

    std::cerr << "VERIFIED " << verified << std::endl;

    if (!verified) {
        std::cerr << "bobIdentityKey:          " << ByteUtil::toHex(bobIdentityKey.serialize()) << std::endl;
        std::cerr << "bobEphemeralPublicKey:   " << ByteUtil::toHex(bobEphemeralPublicKey.serialize()) << std::endl;

        std::cerr << "aliceBasePublicKey:      " << ByteUtil::toHex(aliceBasePublicKey.serialize()) << std::endl;
        std::cerr << "aliceBasePrivateKey:     " << ByteUtil::toHex(aliceBasePrivateKey.serialize()) << std::endl;

        std::cerr << "aliceEphemeralPublicKey: " << ByteUtil::toHex(aliceEphemeralPublicKey.serialize()) << std::endl;
        std::cerr << "aliceEphemeralPrivateKey:" << ByteUtil::toHex(aliceEphemeralPrivateKey.serialize()) << std::endl;
        std::cerr << "aliceIdentityPublicKey:  " << ByteUtil::toHex(aliceIdentityPublicKey.serialize()) << std::endl;
        std::cerr << "aliceIdentityPrivateKey: " << ByteUtil::toHex(aliceIdentityPrivateKey.serialize()) << std::endl;
        std::cerr << "localIdentityKey:        " << ByteUtil::toHex(localIdentityKey.serialize()) << std::endl;
        std::cerr << "remoteIdentityKey:       " << ByteUtil::toHex(remoteIdentityKey.serialize()) << std::endl;
        std::cerr << "receiverChainKey:        " << ByteUtil::toHex(receiverChainKey) << std::endl;
    }
}
