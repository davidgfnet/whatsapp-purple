#include "rootkeytest.h"

#include "byteutil.h"
#include <iostream>

#include "ecc/curve.h"
#include "ecc/djbec.h"
#include "ecc/eckeypair.h"
#include "kdf/hkdf.h"
#include "ratchet/rootkey.h"

RootKeyTest::RootKeyTest()
{
}

void RootKeyTest::testRootKeyDerivationV2()
{
    std::cerr << "testRootKeyDerivationV2" << std::endl;

    ByteArray rootKeySeed  = ByteUtil::fromHex("7ba6debc2bc1bbf91abbc1367404176ca623095b7ec66b45f602d93538942dcc");
    ByteArray alicePublic  = ByteUtil::fromHex("05ee4fa6cdc030df49ecd0ba6cfcffb233d365a27fadbeff77e963fcb16222e13a");
    ByteArray alicePrivate = ByteUtil::fromHex("216822ec67eb38049ebae7b939baeaebb151bbb32db80fd389245ac37a948e50");
    ByteArray bobPublic    = ByteUtil::fromHex("05abb8eb29cc80b47109a2265abe9798485406e32da268934a9555e84757708a30");
    ByteArray nextRoot     = ByteUtil::fromHex("b114f5de28011985e6eba25d50e7ec41a9b02f5693c5c788a63a06d212a2f731");
    ByteArray nextChain    = ByteUtil::fromHex("9d7d2469bc9ae53ee9805aa3264d2499a3ace80f4ccae2da13430c5c55b5ca5f");

    DjbECPublicKey   alicePublicKey = Curve::decodePoint(alicePublic, 0);
    DjbECPrivateKey alicePrivateKey = Curve::decodePrivatePoint(alicePrivate);
    ECKeyPair          aliceKeyPair = ECKeyPair(alicePublicKey, alicePrivateKey);
    DjbECPublicKey     bobPublicKey = Curve::decodePoint(bobPublic, 0);
    RootKey                 rootKey = RootKey(HKDF(2), rootKeySeed);
    std::pair<RootKey, ChainKey> rootKeyChainKeyPair = rootKey.createChain(bobPublicKey, aliceKeyPair);

    RootKey nextRootKey  = rootKeyChainKeyPair.first;
    ChainKey nextChainKey= rootKeyChainKeyPair.second;

    bool verified = rootKey.getKeyBytes() == rootKeySeed
             && nextRootKey.getKeyBytes() == nextRoot
                 && nextChainKey.getKey() == nextChain;

    std::cerr << "VERIFIED " << verified << std::endl;

    if (!verified) {
        std::cerr << "alicePublicKey: " << ByteUtil::toHex(alicePublicKey.serialize()) << std::endl;
        std::cerr << "alicePrivateKey:" << ByteUtil::toHex(alicePrivateKey.serialize()) << std::endl;
        std::cerr << "bobPublicKey:   " << ByteUtil::toHex(bobPublicKey.serialize()) << std::endl;
        std::cerr << "rootKey:        " << ByteUtil::toHex(rootKey.getKeyBytes()) << std::endl;
        std::cerr << "nextRootKey:    " << ByteUtil::toHex(nextRootKey.getKeyBytes()) << std::endl;
        std::cerr << "nextChainKey:   " << ByteUtil::toHex(nextChainKey.getKey()) << std::endl;
    }
}
