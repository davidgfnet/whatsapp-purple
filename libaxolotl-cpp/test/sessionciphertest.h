#ifndef SESSIONCIPHERTEST_H
#define SESSIONCIPHERTEST_H

#include "state/sessionstate.h"
#include "state/sessionrecord.h"

class SessionCipherTest
{
public:
    SessionCipherTest();

    void testBasicSessionV2();
    void testBasicSessionV3();

private:
    void runInteraction(SessionRecord *aliceSessionRecord, SessionRecord *bobSessionRecord);

    void initializeSessionsV2(SessionState *aliceSessionState, SessionState *bobSessionState);
    void initializeSessionsV3(SessionState *aliceSessionState, SessionState *bobSessionState);
};

#endif // SESSIONCIPHERTEST_H
