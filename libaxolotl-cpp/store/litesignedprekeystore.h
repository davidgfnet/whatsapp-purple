#ifndef LITESIGNEDPREKEYSTORE_H
#define LITESIGNEDPREKEYSTORE_H

#include "state/signedprekeystore.h"
#include "state/signedprekeyrecord.h"

#include <vector>
#include <stdint.h>

#include <sqlite/connection.hpp>
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>

class LiteSignedPreKeyStore : public SignedPreKeyStore
{
public:
    LiteSignedPreKeyStore(sqlite::connection &db);
    void clear();

    SignedPreKeyRecord loadSignedPreKey(uint64_t signedPreKeyId) ;
    std::vector<SignedPreKeyRecord> loadSignedPreKeys();
    void storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record);
    bool containsSignedPreKey(uint64_t signedPreKeyId);
    void removeSignedPreKey(uint64_t signedPreKeyId);

private:
    sqlite::connection &_db;
};

#endif // LITESIGNEDPREKEYSTORE_H
