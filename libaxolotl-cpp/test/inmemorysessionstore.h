#ifndef INMEMORYSESSIONSTORE_H
#define INMEMORYSESSIONSTORE_H

#include "state/sessionstore.h"
#include "state/sessionrecord.h"

#include <utility>
#include <map>
#include <vector>
#include "byteutil.h"

typedef std::pair<uint64_t, int> SessionsKeyPair;

class InMemorySessionStore : public SessionStore
{
public:
    InMemorySessionStore();
    SessionRecord *loadSession(uint64_t recipientId, int deviceId);
    std::vector<int> getSubDeviceSessions(uint64_t recipientId);
    void storeSession(uint64_t recipientId, int deviceId, SessionRecord *record);
    bool containsSession(uint64_t recipientId, int deviceId);
    void deleteSession(uint64_t recipientId, int deviceId);
    void deleteAllSessions(uint64_t recipientId);

private:
    std::map<SessionsKeyPair, ByteArray> sessions;
};

#endif // INMEMORYSESSIONSTORE_H
