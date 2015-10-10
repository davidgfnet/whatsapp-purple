#include "chainkeytest.h"

#include "byteutil.h"
#include <iostream>

#include "ratchet/chainkey.h"
#include "kdf/hkdf.h"

ChainKeyTest::ChainKeyTest()
{
}

void ChainKeyTest::testChainKeyDerivationV2()
{
    std::cerr << "testChainKeyDerivationV2" << std::endl;

    ByteArray         seed = ByteUtil::fromHex("8ab72d6f4cc5ac0d387eaf463378ddb28edd07385b1cb01250c715982e7ad48f");
    ByteArray   messageKey = ByteUtil::fromHex("02a9aa6c7dbd64f9d3aa92f92a277bf54609dadf0b00828acfc61e3c724b84a7");
    ByteArray       macKey = ByteUtil::fromHex("bfbe5efb603030526742e3ee89c7024e884e440f1ff376bb2317b2d64deb7c83");
    ByteArray nextChainKey = ByteUtil::fromHex("28e8f8fee54b801eef7c5cfb2f17f32c7b334485bbb70fac6ec10342a246d15d");

    HKDF kdf(2);
    ChainKey chainKey(kdf, seed, 0);

    bool verified = chainKey.getKey() == seed
            && chainKey.getMessageKeys().getCipherKey() == messageKey
            && chainKey.getMessageKeys().getMacKey() == macKey
            && chainKey.getNextChainKey().getKey() == nextChainKey
            && chainKey.getIndex() == 0
            && chainKey.getMessageKeys().getCounter() == 0
            && chainKey.getNextChainKey().getIndex() == 1
            && chainKey.getNextChainKey().getMessageKeys().getCounter() == 1;

    std::cerr << "VERIFIED " << verified << std::endl;

    if (!verified) {
        std::cerr << "getKey:      " << ByteUtil::toHex(chainKey.getKey()) << std::endl;
        std::cerr << "getCipherKey:" << ByteUtil::toHex(chainKey.getMessageKeys().getCipherKey()) << std::endl;
        std::cerr << "getMacKey:   " << ByteUtil::toHex(chainKey.getMessageKeys().getMacKey()) << std::endl;
        std::cerr << "nextKey:     " << ByteUtil::toHex(chainKey.getNextChainKey().getKey()) << std::endl;
        std::cerr << "getIndex:    " << chainKey.getIndex() << std::endl;
        std::cerr << "getCounter:  " << chainKey.getMessageKeys().getCounter() << std::endl;
        std::cerr << "nextIndex:   " << chainKey.getNextChainKey().getIndex() << std::endl;
        std::cerr << "nextCounter: " << chainKey.getNextChainKey().getMessageKeys().getCounter() << std::endl;
    }
}
