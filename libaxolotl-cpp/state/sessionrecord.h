#ifndef SESSIONRECORD_H
#define SESSIONRECORD_H

#include "sessionstate.h"

#include <vector>
#include "byteutil.h"

class SessionRecord
{
public:
    SessionRecord();
    SessionRecord(SessionState *sessionState);
    SessionRecord(const ByteArray &serialized);

    bool hasSessionState(int version, const ByteArray &aliceBaseKey);
    SessionState *getSessionState();
    std::vector<SessionState*> getPreviousSessionStates();
    bool isFresh() const;
    void promoteState(SessionState *promotedState);
    void archiveCurrentState();
    void setState(SessionState *sessionState);
    ByteArray serialize() const;

private:
    static const int ARCHIVED_STATES_MAX_LENGTH;
    SessionState *sessionState;
    std::vector<SessionState*> previousStates;
    bool fresh;
};

#endif // SESSIONRECORD_H
