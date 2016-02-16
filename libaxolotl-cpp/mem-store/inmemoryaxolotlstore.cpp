#include "inmemoryaxolotlstore.h"

#include <iostream>
#include <vector>

#ifdef DEBUG
	#define DEBUG_PRINT(a) std::clog << a << std::endl;
#else
	#define DEBUG_PRINT(a)
#endif

InMemoryAxolotlStore::InMemoryAxolotlStore()
{
}

IdentityKeyPair InMemoryAxolotlStore::getIdentityKeyPair()
{
	return identityKeyStore.getIdentityKeyPair();
}

unsigned int InMemoryAxolotlStore::getLocalRegistrationId()
{
	auto r = identityKeyStore.getLocalRegistrationId();
	DEBUG_PRINT("getLocalRegistrationId " << r);
	return r;
}

void InMemoryAxolotlStore::storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair) {
	DEBUG_PRINT("storeLocalData " << registrationId);
	identityKeyStore.storeLocalData(registrationId, identityKeyPair);
}

void InMemoryAxolotlStore::removeIdentity(uint64_t recipientId) {
	DEBUG_PRINT("removeIdentity " << recipientId);
	identityKeyStore.removeIdentity(recipientId);
}

void InMemoryAxolotlStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
	DEBUG_PRINT("saveIdentity " << recipientId);
	identityKeyStore.saveIdentity(recipientId, identityKey);
}

bool InMemoryAxolotlStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
	//bool r = identityKeyStore.isTrustedIdentity(recipientId, identityKey);
	//DEBUG_PRINT("isTrustedIdentity " << recipientId << " " << r);
	return true;
}

PreKeyRecord InMemoryAxolotlStore::loadPreKey(uint64_t preKeyId)
{
	DEBUG_PRINT("loadPreKey " << preKeyId);
	return preKeyStore.loadPreKey(preKeyId);
}

void InMemoryAxolotlStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
	DEBUG_PRINT("storePreKey " << preKeyId);
	preKeyStore.storePreKey(preKeyId, record);
}

bool InMemoryAxolotlStore::containsPreKey(uint64_t preKeyId)
{
	bool r = preKeyStore.containsPreKey(preKeyId);
	DEBUG_PRINT("containsPreKey " << preKeyId << " " << r);
	return r;
}

void InMemoryAxolotlStore::removePreKey(uint64_t preKeyId)
{
	DEBUG_PRINT("removePreKey " << preKeyId);
	preKeyStore.removePreKey(preKeyId);
}

SessionRecord *InMemoryAxolotlStore::loadSession(uint64_t recipientId, int deviceId)
{
	return sessionStore.loadSession(recipientId, deviceId);
}

std::vector<int> InMemoryAxolotlStore::getSubDeviceSessions(uint64_t recipientId)
{
	return sessionStore.getSubDeviceSessions(recipientId);
}

void InMemoryAxolotlStore::storeSession(uint64_t recipientId, int deviceId, SessionRecord *record)
{
	DEBUG_PRINT("storeSession " << recipientId << " " << deviceId);
	sessionStore.storeSession(recipientId, deviceId, record);
}

bool InMemoryAxolotlStore::containsSession(uint64_t recipientId, int deviceId)
{
	bool r = sessionStore.containsSession(recipientId, deviceId);
	DEBUG_PRINT("containsSession " << recipientId << " " << deviceId << " " << r);
    return r;
}

void InMemoryAxolotlStore::deleteSession(uint64_t recipientId, int deviceId)
{
	DEBUG_PRINT("deleteSession " << recipientId << " " << deviceId);
	sessionStore.deleteSession(recipientId, deviceId);
}

void InMemoryAxolotlStore::deleteAllSessions(uint64_t recipientId)
{
	DEBUG_PRINT("deleteAllSessions " << recipientId);
	sessionStore.deleteAllSessions(recipientId);
}

SignedPreKeyRecord InMemoryAxolotlStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
	DEBUG_PRINT("loadSignedPreKey " << signedPreKeyId);
	return signedPreKeyStore.loadSignedPreKey(signedPreKeyId);
}

std::vector<SignedPreKeyRecord> InMemoryAxolotlStore::loadSignedPreKeys()
{
	return signedPreKeyStore.loadSignedPreKeys();
}

void InMemoryAxolotlStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
	DEBUG_PRINT("storeSignedPreKey " << signedPreKeyId);
	signedPreKeyStore.storeSignedPreKey(signedPreKeyId, record);
}

bool InMemoryAxolotlStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
	bool r = signedPreKeyStore.containsSignedPreKey(signedPreKeyId);
	DEBUG_PRINT("containsSignedPreKey " << signedPreKeyId << " " << r);
	return r;
}

void InMemoryAxolotlStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
	DEBUG_PRINT("removeSignedPreKey " << signedPreKeyId);
	signedPreKeyStore.removeSignedPreKey(signedPreKeyId);
}

void InMemoryAxolotlStore::storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record) {
	senderKeyStore.storeSenderKey(senderKeyId, record);
}

SenderKeyRecord InMemoryAxolotlStore::loadSenderKey(const ByteArray &senderKeyId) const {
	return senderKeyStore.loadSenderKey(senderKeyId);
}


std::string InMemoryAxolotlStore::serialize() const {
	return 
		identityKeyStore.serialize() +
		preKeyStore.serialize() +
		sessionStore.serialize() +
		signedPreKeyStore.serialize() +
		senderKeyStore.serialize();
}

InMemoryAxolotlStore::InMemoryAxolotlStore(std::string data) {
	if (data.size()) {
		Unserializer uns(data);
		identityKeyStore = InMemoryIdentityKeyStore(uns);
		preKeyStore = InMemoryPreKeyStore(uns);
		sessionStore = InMemorySessionStore(uns);
		signedPreKeyStore = InMemorySignedPreKeyStore(uns);
		senderKeyStore = InMemorySenderKeyStore(uns);
	}
}



