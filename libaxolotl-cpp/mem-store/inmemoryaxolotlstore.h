#ifndef INMEMORYAXOLOTLSTORE_H
#define INMEMORYAXOLOTLSTORE_H

#include <iostream>
#include "state/axolotlstore.h"

#include "inmemoryidentitykeystore.h"
#include "inmemoryprekeystore.h"
#include "inmemorysessionstore.h"
#include "inmemorysignedprekeystore.h"
#include "inmemorysenderkeystore.h"

#include <vector>

#include "state/identitykeystore.h"
#include "identitykeypair.h"
#include "util/keyhelper.h"
#include "state/prekeystore.h"
#include "state/sessionstore.h"
#include "state/sessionrecord.h"
#include "state/signedprekeystore.h"

class InMemoryAxolotlStore : public AxolotlStore
{
public:
	InMemoryAxolotlStore();
	InMemoryAxolotlStore(std::string data);

	IdentityKeyPair getIdentityKeyPair();
	unsigned int	getLocalRegistrationId();
	void			saveIdentity(uint64_t recipientId, const IdentityKey &identityKey);
	bool			isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey);
	void			removeIdentity(uint64_t recipientId);
	void			storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair);

	PreKeyRecord loadPreKey(uint64_t preKeyId);
	void		 storePreKey(uint64_t preKeyId, const PreKeyRecord &record);
	bool		 containsPreKey(uint64_t preKeyId);
	void		 removePreKey(uint64_t preKeyId);
	int  countPreKeys() { return preKeyStore.countPreKeys(); }

	SessionRecord *loadSession(uint64_t recipientId, int deviceId);
	std::vector<int> getSubDeviceSessions(uint64_t recipientId);
	void storeSession(uint64_t recipientId, int deviceId, SessionRecord *record);
	bool containsSession(uint64_t recipientId, int deviceId);
	void deleteSession(uint64_t recipientId, int deviceId);
	void deleteAllSessions(uint64_t recipientId);

	SignedPreKeyRecord loadSignedPreKey(uint64_t signedPreKeyId);
	std::vector<SignedPreKeyRecord> loadSignedPreKeys();
	void storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record);
	bool containsSignedPreKey(uint64_t signedPreKeyId);
	void removeSignedPreKey(uint64_t signedPreKeyId);

    void storeSenderKey(const ByteArray &senderKeyId, SenderKeyRecord *record);
    SenderKeyRecord loadSenderKey(const ByteArray &senderKeyId) const;

	std::string serialize() const;

private:
	InMemoryIdentityKeyStore  identityKeyStore;
	InMemoryPreKeyStore	   preKeyStore;
	InMemorySessionStore	  sessionStore;
	InMemorySignedPreKeyStore signedPreKeyStore;
	InMemorySenderKeyStore senderKeyStore;
};

#endif // INMEMORYAXOLOTLSTORE_H
