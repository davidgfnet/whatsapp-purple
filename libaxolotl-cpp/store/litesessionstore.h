#ifndef LITESESSIONSTORE_H
#define LITESESSIONSTORE_H

#include "state/sessionstore.h"

#include <vector>
#include <stdint.h>

#include <sqlite/connection.hpp>
#include <sqlite/execute.hpp>
#include <sqlite/query.hpp>

class LiteSessionStore : public SessionStore
{
public:
    LiteSessionStore(sqlite::connection &db);
    void clear();

    SessionRecord *loadSession(uint64_t recipientId, int deviceId);
    std::vector<int> getSubDeviceSessions(uint64_t recipientId);
    void storeSession(uint64_t recipientId, int deviceId, SessionRecord *record);
    bool containsSession(uint64_t recipientId, int deviceId);
    void deleteSession(uint64_t recipientId, int deviceId);
    void deleteAllSessions(uint64_t recipientId);

private:
    sqlite::connection &_db;
};

#endif // LITESESSIONSTORE_H
