#include "keyhelper.h"

#include <openssl/rand.h>
#include <string.h>

#include "eckeypair.h"
#include "curve.h"

uint64_t KeyHelper::getRandomFFFF()
{
	RAND_poll();

	unsigned char buff1[2];
	memset(buff1, 0, 2);
	RAND_bytes(buff1, 2);
	uint64_t rand1 = ((unsigned int)buff1[1] << 8) + (unsigned int)buff1[0];

	return rand1;
}

uint64_t KeyHelper::getRandom7FFFFFFF()
{
	RAND_poll();

	uint64_t rand2 = 0;
	for (int i = 0; i < 0x80; i++) {
		unsigned char buff0[3];
		memset(buff0, 0, 3);
		RAND_bytes(buff0, 3);
		rand2 += ((unsigned int)buff0[2] << 16);
		rand2 += ((unsigned int)buff0[1] << 8);
		rand2 += (unsigned int)buff0[0];
	}

	return rand2;
}

uint64_t KeyHelper::getRandomFFFFFFFF()
{
	RAND_poll();

	unsigned char buff1[4];
	memset(buff1, 0, 4);
	RAND_bytes(buff1, 4);
	uint64_t rand1 = ((uint64_t)buff1[3] << 24) + ((uint64_t)buff1[2] << 16)
					   +  ((uint64_t)buff1[1] << 8)  +  (uint64_t)buff1[0];

	return rand1;
}

ByteArray KeyHelper::getRandomBytes(int bytes)
{
	RAND_poll();

	unsigned char buff1[bytes];
	memset(buff1, 0, bytes);
	RAND_bytes(buff1, bytes);
	return ByteArray((const char *)buff1, bytes);
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
	return getRandomFFFFFFFF();
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
	RAND_poll();

	unsigned char buff1[32];
	memset(buff1, 0, 32);
	RAND_bytes(buff1, 32);

	return ByteArray ((const char*)buff1, 32);
}

unsigned long KeyHelper::generateSenderKeyId()
{
	return getRandom7FFFFFFF();
}
