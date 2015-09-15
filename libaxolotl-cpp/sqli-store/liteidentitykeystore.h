#ifndef LITEIDENTITYKEYSTORE_H
#define LITEIDENTITYKEYSTORE_H

#include "state/identitykeystore.h"

#include <vector>
#include <stdint.h>

#include <sqlite/connection.hpp>
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>

class LiteIdentityKeyStore : public IdentityKeyStore
{
public:
    LiteIdentityKeyStore(sqlite::connection &db);
    void clear();

    IdentityKeyPair getIdentityKeyPair();
    unsigned int getLocalRegistrationId();
    void            storeLocalData(uint64_t registrationId, const IdentityKeyPair identityKeyPair);
    void            saveIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    bool            isTrustedIdentity(uint64_t recipientId, const IdentityKey &identityKey);
    void            removeIdentity(uint64_t recipientId);

private:
    sqlite::connection &_db;
};

#endif // LITEIDENTITYKEYSTORE_H
