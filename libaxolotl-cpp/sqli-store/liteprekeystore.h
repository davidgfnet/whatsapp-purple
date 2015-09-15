#ifndef LITEPREKEYSTORE_H
#define LITEPREKEYSTORE_H

#include "state/prekeystore.h"
#include "state/prekeyrecord.h"

#include <sqlite/connection.hpp>
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>

class LitePreKeyStore : public PreKeyStore
{
public:
    LitePreKeyStore(sqlite::connection &db);
    void clear();

    PreKeyRecord loadPreKey(uint64_t preKeyId);
    void         storePreKey(uint64_t preKeyId, const PreKeyRecord &record);
    bool         containsPreKey(uint64_t preKeyId);
    void         removePreKey(uint64_t preKeyId);
    int          countPreKeys();

private:
    sqlite::connection &_db;
};

#endif // LITEPREKEYSTORE_H
