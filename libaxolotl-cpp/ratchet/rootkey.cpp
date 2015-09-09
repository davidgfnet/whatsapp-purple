#include "rootkey.h"
#include "curve.h"
#include "derivedrootsecrets.h"
#include <utility>

RootKey::RootKey()
{

}

RootKey::RootKey(const HKDF &kdf, const ByteArray &key)
{
    this->kdf = kdf;
    this->key = key;
}

ByteArray RootKey::getKeyBytes() const
{
    return key;
}

std::pair<RootKey, ChainKey> RootKey::createChain(const DjbECPublicKey &theirRatchetKey, const ECKeyPair &ourRatchetKey)
{
    ByteArray sharedSecret = Curve::calculateAgreement(theirRatchetKey, ourRatchetKey.getPrivateKey());
    ByteArray derivedSecretBytes = kdf.deriveSecrets(sharedSecret, ByteArray("WhisperRatchet"), DerivedRootSecrets::SIZE, key);
    DerivedRootSecrets derivedSecrets(derivedSecretBytes);

    RootKey newRootKey(kdf, derivedSecrets.getRootKey());
    ChainKey newChainKey(kdf, derivedSecrets.getChainKey(), 0);

    std::pair<RootKey, ChainKey> pair;
    pair.first = newRootKey;
    pair.second = newChainKey;

    return pair;
}
