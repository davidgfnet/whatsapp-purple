#include <iostream>
#include "liteaxolotlstore.h"

LiteAxolotlStore::LiteAxolotlStore(const std::string &connection)
 : _db(connection.c_str())
{
    initStore();
    _connection = connection;
}

void LiteAxolotlStore::initStore()
{
    identityKeyStore = new LiteIdentityKeyStore(_db);
    preKeyStore = new LitePreKeyStore(_db);
    sessionStore = new LiteSessionStore(_db);
    signedPreKeyStore = new LiteSignedPreKeyStore(_db);
}

void LiteAxolotlStore::clear()
{
    identityKeyStore->clear();
    preKeyStore->clear();
    sessionStore->clear();
    signedPreKeyStore->clear();
}

IdentityKeyPair LiteAxolotlStore::getIdentityKeyPair()
{
    return identityKeyStore->getIdentityKeyPair();
}

unsigned int LiteAxolotlStore::getLocalRegistrationId()
{
    auto r = identityKeyStore->getLocalRegistrationId();
	std::cerr << "getLocalRegistrationId " << r << std::endl;
	return r;
}

void LiteAxolotlStore::removeIdentity(uint64_t recipientId)
{
	std::cerr << "removeIdentity " << recipientId << std::endl;
    identityKeyStore->removeIdentity(recipientId);
}

void LiteAxolotlStore::storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair)
{
	std::cerr << "storeLocalData " << registrationId << std::endl;
    identityKeyStore->storeLocalData(registrationId, identityKeyPair);
}

void LiteAxolotlStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
	std::cerr << "saveIdentity " << recipientId << std::endl;
    identityKeyStore->saveIdentity(recipientId, identityKey);
}

bool LiteAxolotlStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    bool r = identityKeyStore->isTrustedIdentity(recipientId, identityKey);
	std::cerr << "isTrustedIdentity " << recipientId << " " << r << std::endl;
	return r;
}

PreKeyRecord LiteAxolotlStore::loadPreKey(uint64_t preKeyId)
{
	std::cerr << "loadPreKey " << preKeyId << std::endl;
    return preKeyStore->loadPreKey(preKeyId);
}

void LiteAxolotlStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
	std::cerr << "storePreKey " << preKeyId << std::endl;
    preKeyStore->storePreKey(preKeyId, record);
}

bool LiteAxolotlStore::containsPreKey(uint64_t preKeyId)
{
    bool r = preKeyStore->containsPreKey(preKeyId);
	std::cerr << "containsPreKey " << preKeyId << " " << r << std::endl;
	return r;
}

void LiteAxolotlStore::removePreKey(uint64_t preKeyId)
{
	std::cerr << "removePreKey " << preKeyId << std::endl;
    preKeyStore->removePreKey(preKeyId);
}

int LiteAxolotlStore::countPreKeys()
{
    return preKeyStore->countPreKeys();
}

SessionRecord *LiteAxolotlStore::loadSession(uint64_t recipientId, int deviceId)
{
    return sessionStore->loadSession(recipientId, deviceId);
}

std::vector<int> LiteAxolotlStore::getSubDeviceSessions(uint64_t recipientId)
{
    return sessionStore->getSubDeviceSessions(recipientId);
}

void LiteAxolotlStore::storeSession(uint64_t recipientId, int deviceId, SessionRecord *record)
{
	std::cerr << "storeSession " << recipientId << " " << deviceId << std::endl;
    sessionStore->storeSession(recipientId, deviceId, record);
}

bool LiteAxolotlStore::containsSession(uint64_t recipientId, int deviceId)
{
    bool r = sessionStore->containsSession(recipientId, deviceId);
	std::cerr << "containsSession " << recipientId << " " << deviceId << " " << r << std::endl;
    return r;
}

void LiteAxolotlStore::deleteSession(uint64_t recipientId, int deviceId)
{
	std::cerr << "deleteSession " << recipientId << " " << deviceId << std::endl;
    sessionStore->deleteSession(recipientId, deviceId);
}

void LiteAxolotlStore::deleteAllSessions(uint64_t recipientId)
{
	std::cerr << "deleteAllSessions " << recipientId << std::endl;
    sessionStore->deleteAllSessions(recipientId);
}

SignedPreKeyRecord LiteAxolotlStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
	std::cerr << "loadSignedPreKey " << signedPreKeyId << std::endl;
    return signedPreKeyStore->loadSignedPreKey(signedPreKeyId);
}

std::vector<SignedPreKeyRecord> LiteAxolotlStore::loadSignedPreKeys()
{
    return signedPreKeyStore->loadSignedPreKeys();
}

void LiteAxolotlStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
	std::cerr << "storeSignedPreKey " << signedPreKeyId << std::endl;
    signedPreKeyStore->storeSignedPreKey(signedPreKeyId, record);
}

bool LiteAxolotlStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
    bool r = signedPreKeyStore->containsSignedPreKey(signedPreKeyId);
	std::cerr << "containsSignedPreKey " << signedPreKeyId << " " << r << std::endl;
	return r;
}

void LiteAxolotlStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
	std::cerr << "removeSignedPreKey " << signedPreKeyId << std::endl;
    signedPreKeyStore->removeSignedPreKey(signedPreKeyId);
}
