#ifndef CHAINKEY_H
#define CHAINKEY_H

#include "hkdf.h"
#include "messagekeys.h"
#include "derivedmessagesecrets.h"

class ChainKey
{
public:
    ChainKey();
    ChainKey(const HKDF &kdf, const ByteArray &key, unsigned int index);

    ByteArray getKey() const;
    unsigned int getIndex() const;
    ByteArray getBaseMaterial(const ByteArray &seed) const;
    ChainKey getNextChainKey() const;
    MessageKeys getMessageKeys() const;

    static const ByteArray MESSAGE_KEY_SEED;
    static const ByteArray CHAIN_KEY_SEED;

private:
    HKDF kdf;
    ByteArray key;
    unsigned int index;

};

#endif // CHAINKEY_H
