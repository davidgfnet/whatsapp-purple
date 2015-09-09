#ifndef SESSIONSTORE_H
#define SESSIONSTORE_H

#include "sessionrecord.h"

class SessionStore
{
public:
    virtual SessionRecord *loadSession(uint64_t recipientId, int deviceId) = 0;
    virtual std::vector<int> getSubDeviceSessions(uint64_t recipientId) = 0;
    virtual void storeSession(uint64_t recipientId, int deviceId, SessionRecord *record) = 0;
    virtual bool containsSession(uint64_t recipientId, int deviceId) = 0;
    virtual void deleteSession(uint64_t recipientId, int deviceId) = 0;
    virtual void deleteAllSessions(uint64_t recipientId) = 0;
};

#endif // SESSIONSTORE_H
