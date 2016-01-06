#include "chainkey.h"

void HMAC_SHA256(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest);

const ByteArray ChainKey::MESSAGE_KEY_SEED = ByteArray("\x01");
const ByteArray ChainKey::CHAIN_KEY_SEED = ByteArray("\x02");

ChainKey::ChainKey()
{

}

ChainKey::ChainKey(const HKDF &kdf, const ByteArray &key, unsigned int index)
{
    this->kdf = kdf;
    this->key = key;
    this->index = index;
}

ByteArray ChainKey::getKey() const
{
    return key;
}

unsigned int ChainKey::getIndex() const
{
    return index;
}

ByteArray ChainKey::getBaseMaterial(const ByteArray &seed) const
{
    unsigned char out[32];
	HMAC_SHA256((unsigned char*)seed.c_str(), seed.size(), (unsigned char*)key.c_str(), key.size(), out);
    return ByteArray((const char*)out, 32);
}

ChainKey ChainKey::getNextChainKey() const
{
    ByteArray nextKey = getBaseMaterial(CHAIN_KEY_SEED);
    return ChainKey(kdf, nextKey, index + 1);
}

MessageKeys ChainKey::getMessageKeys() const
{
    ByteArray inputKeyMaterial = getBaseMaterial(MESSAGE_KEY_SEED);
    ByteArray keyMaterialBytes = kdf.deriveSecrets(inputKeyMaterial, ByteArray("WhisperMessageKeys"), DerivedMessageSecrets::SIZE);
    DerivedMessageSecrets keyMaterial(keyMaterialBytes);
    return MessageKeys(keyMaterial.getCipherKey(), keyMaterial.getMacKey(), keyMaterial.getIv(), index);
}
