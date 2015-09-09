#ifndef __LITEAXOLOTLSTORE_H
#define __LITEAXOLOTLSTORE_H

#include "state/axolotlstore.h"
#include "liteidentitykeystore.h"
#include "liteprekeystore.h"
#include "litesessionstore.h"
#include "litesignedprekeystore.h"

#include <sqlite/connection.hpp>

#include <string>
#include <vector>
#include <stdint.h>

class LiteAxolotlStore : public AxolotlStore
{
public:
    LiteAxolotlStore(const std::string &connection);
    void clear();

    IdentityKeyPair getIdentityKeyPair();
    unsigned int    getLocalRegistrationId();
    void            storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair);
    void            saveIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    bool            isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    void            removeIdentity(uint64_t recipientId);

    PreKeyRecord loadPreKey(uint64_t preKeyId);
    void         storePreKey(uint64_t preKeyId, const PreKeyRecord &record);
    bool         containsPreKey(uint64_t preKeyId);
    void         removePreKey(uint64_t preKeyId);
    int          countPreKeys();

    SessionRecord *  loadSession(uint64_t recipientId, int deviceId);
    std::vector<int> getSubDeviceSessions(uint64_t recipientId);
    void             storeSession(uint64_t recipientId, int deviceId, SessionRecord *record);
    bool             containsSession(uint64_t recipientId, int deviceId);
    void             deleteSession(uint64_t recipientId, int deviceId);
    void             deleteAllSessions(uint64_t recipientId);

    SignedPreKeyRecord        loadSignedPreKey(uint64_t signedPreKeyId);
    std::vector <SignedPreKeyRecord> loadSignedPreKeys();
    void                      storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record);
    bool                      containsSignedPreKey(uint64_t signedPreKeyId);
    void                      removeSignedPreKey(uint64_t signedPreKeyId);

private:
    void initStore();

    sqlite::connection _db;
    std::string _connection;

    LiteIdentityKeyStore    *identityKeyStore;
    LitePreKeyStore         *preKeyStore;
    LiteSessionStore        *sessionStore;
    LiteSignedPreKeyStore   *signedPreKeyStore;
};

#endif // LITEAXOLOTLSTORE_H
