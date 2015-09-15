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
    return identityKeyStore->getLocalRegistrationId();
}

void LiteAxolotlStore::removeIdentity(uint64_t recipientId)
{
    identityKeyStore->removeIdentity(recipientId);
}

void LiteAxolotlStore::storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair)
{
    identityKeyStore->storeLocalData(registrationId, identityKeyPair);
}

void LiteAxolotlStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    identityKeyStore->saveIdentity(recipientId, identityKey);
}

bool LiteAxolotlStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    return identityKeyStore->isTrustedIdentity(recipientId, identityKey);
}

PreKeyRecord LiteAxolotlStore::loadPreKey(uint64_t preKeyId)
{
    return preKeyStore->loadPreKey(preKeyId);
}

void LiteAxolotlStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
    preKeyStore->storePreKey(preKeyId, record);
}

bool LiteAxolotlStore::containsPreKey(uint64_t preKeyId)
{
    return preKeyStore->containsPreKey(preKeyId);
}

void LiteAxolotlStore::removePreKey(uint64_t preKeyId)
{
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
    sessionStore->storeSession(recipientId, deviceId, record);
}

bool LiteAxolotlStore::containsSession(uint64_t recipientId, int deviceId)
{
    return sessionStore->containsSession(recipientId, deviceId);
}

void LiteAxolotlStore::deleteSession(uint64_t recipientId, int deviceId)
{
    sessionStore->deleteSession(recipientId, deviceId);
}

void LiteAxolotlStore::deleteAllSessions(uint64_t recipientId)
{
    sessionStore->deleteAllSessions(recipientId);
}

SignedPreKeyRecord LiteAxolotlStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
    return signedPreKeyStore->loadSignedPreKey(signedPreKeyId);
}

std::vector<SignedPreKeyRecord> LiteAxolotlStore::loadSignedPreKeys()
{
    return signedPreKeyStore->loadSignedPreKeys();
}

void LiteAxolotlStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
    signedPreKeyStore->storeSignedPreKey(signedPreKeyId, record);
}

bool LiteAxolotlStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
    return signedPreKeyStore->containsSignedPreKey(signedPreKeyId);
}

void LiteAxolotlStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
    signedPreKeyStore->removeSignedPreKey(signedPreKeyId);
}
