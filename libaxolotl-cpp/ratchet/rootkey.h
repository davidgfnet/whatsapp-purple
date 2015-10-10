#ifndef ROOTKEY_H
#define ROOTKEY_H

#include "../kdf/hkdf.h"
#include "chainkey.h"
#include "../ecc/eckeypair.h"

#include "byteutil.h"
#include <utility>

class RootKey
{
public:
    RootKey();
    RootKey(const HKDF &kdf, const ByteArray &key);

    ByteArray getKeyBytes() const;
    std::pair<RootKey, ChainKey> createChain(const DjbECPublicKey &theirRatchetKey, const ECKeyPair &ourRatchetKey);

private:
    HKDF kdf;
    ByteArray key;

};

#endif // ROOTKEY_H
