#include "inmemoryaxolotlstore.h"

#include <iostream>
#include <vector>

InMemoryAxolotlStore::InMemoryAxolotlStore()
{
}

IdentityKeyPair InMemoryAxolotlStore::getIdentityKeyPair()
{
    return identityKeyStore.getIdentityKeyPair();
}

unsigned int InMemoryAxolotlStore::getLocalRegistrationId()
{
    return identityKeyStore.getLocalRegistrationId();
}

void InMemoryAxolotlStore::saveIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    identityKeyStore.saveIdentity(recipientId, identityKey);
}

bool InMemoryAxolotlStore::isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey)
{
    return identityKeyStore.isTrustedIdentity(recipientId, identityKey);
}

PreKeyRecord InMemoryAxolotlStore::loadPreKey(uint64_t preKeyId)
{
    return preKeyStore.loadPreKey(preKeyId);
}

void InMemoryAxolotlStore::storePreKey(uint64_t preKeyId, const PreKeyRecord &record)
{
    preKeyStore.storePreKey(preKeyId, record);
}

bool InMemoryAxolotlStore::containsPreKey(uint64_t preKeyId)
{
    return preKeyStore.containsPreKey(preKeyId);
}

void InMemoryAxolotlStore::removePreKey(uint64_t preKeyId)
{
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
    sessionStore.storeSession(recipientId, deviceId, record);
}

bool InMemoryAxolotlStore::containsSession(uint64_t recipientId, int deviceId)
{
    return sessionStore.containsSession(recipientId, deviceId);
}

void InMemoryAxolotlStore::deleteSession(uint64_t recipientId, int deviceId)
{
    sessionStore.deleteSession(recipientId, deviceId);
}

void InMemoryAxolotlStore::deleteAllSessions(uint64_t recipientId)
{
    sessionStore.deleteAllSessions(recipientId);
}

SignedPreKeyRecord InMemoryAxolotlStore::loadSignedPreKey(uint64_t signedPreKeyId)
{
    return signedPreKeyStore.loadSignedPreKey(signedPreKeyId);
}

std::vector<SignedPreKeyRecord> InMemoryAxolotlStore::loadSignedPreKeys()
{
    return signedPreKeyStore.loadSignedPreKeys();
}

void InMemoryAxolotlStore::storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record)
{
    signedPreKeyStore.storeSignedPreKey(signedPreKeyId, record);
}

bool InMemoryAxolotlStore::containsSignedPreKey(uint64_t signedPreKeyId)
{
    return signedPreKeyStore.containsSignedPreKey(signedPreKeyId);
}

void InMemoryAxolotlStore::removeSignedPreKey(uint64_t signedPreKeyId)
{
    signedPreKeyStore.removeSignedPreKey(signedPreKeyId);
}
