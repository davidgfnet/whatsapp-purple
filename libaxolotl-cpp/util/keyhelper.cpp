#include "keyhelper.h"

#include <string.h>

#include "eckeypair.h"
#include "curve.h"

uint64_t KeyHelper::getRandomFFFF()
{
	return getRandomFFFFFFFF() & 0xFFFF;
}

uint64_t KeyHelper::getRandom7FFFFFFF()
{
	return getRandomFFFFFFFF() & 0x7FFFFFFF;
}

uint64_t KeyHelper::getRandomFFFFFFFF()
{
	return (rand() ^ (rand() << 8) ^ (rand() << 16) ^ (rand() << 24)) & 0xFFFFFFFF;
}

ByteArray KeyHelper::getRandomBytes(int bytes)
{
	unsigned char buff[bytes];
	for (unsigned i = 0; i < bytes; i++)
		buff[i] = rand();
	return ByteArray((const char *)buff, bytes);
}

IdentityKeyPair KeyHelper::generateIdentityKeyPair()
{
	ECKeyPair keyPair = Curve::generateKeyPair();
	IdentityKey publicKey(keyPair.getPublicKey());
	IdentityKeyPair identityKeyPair(publicKey, keyPair.getPrivateKey());
	return identityKeyPair;
}

uint64_t KeyHelper::generateRegistrationId()
{
	return getRandom7FFFFFFF();
}

std::vector<PreKeyRecord> KeyHelper::generatePreKeys(uint64_t start, unsigned int count)
{
	std::vector<PreKeyRecord> results;
	for (unsigned int i = 0; i < count; i++) {
		uint64_t preKeyId = ((start + i - 1) % (0xFFFFFE)) + 1;
		results.push_back(PreKeyRecord(preKeyId, Curve::generateKeyPair()));
	}
	return results;
}

SignedPreKeyRecord KeyHelper::generateSignedPreKey(const IdentityKeyPair &identityKeyPair, uint64_t signedPreKeyId)
{
	ECKeyPair keyPair = Curve::generateKeyPair();
	ByteArray signature = Curve::calculateSignature(identityKeyPair.getPrivateKey(), keyPair.getPublicKey().serialize());
	return SignedPreKeyRecord(signedPreKeyId, ((uint64_t)time(0)) * 1000, keyPair, signature);
}

ECKeyPair KeyHelper::generateSenderSigningKey()
{
	return Curve::generateKeyPair();
}

ByteArray KeyHelper::generateSenderKey()
{
	return getRandomBytes(32);
}

unsigned long KeyHelper::generateSenderKeyId()
{
	return getRandom7FFFFFFF();
}
