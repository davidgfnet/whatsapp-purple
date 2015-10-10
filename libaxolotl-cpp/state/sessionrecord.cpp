#include "sessionrecord.h"

const int SessionRecord::ARCHIVED_STATES_MAX_LENGTH = 50;

SessionRecord::SessionRecord()
{
    fresh = true;
    this->sessionState = new SessionState();
}

SessionRecord::SessionRecord(SessionState *sessionState)
{
    this->sessionState = sessionState;
    fresh = false;
}

SessionRecord::SessionRecord(const ByteArray &serialized)
{
    textsecure::RecordStructure record;
    record.ParsePartialFromArray(serialized.c_str(), serialized.size());
    sessionState = new SessionState(record.currentsession());
    fresh = false;

    for (int i = 0; i < record.previoussessions_size(); i++) {
        previousStates.push_back(new SessionState(record.previoussessions(i)));
    }
}

bool SessionRecord::hasSessionState(int version, const ByteArray &aliceBaseKey)
{
    if (sessionState->getSessionVersion() == version
            && aliceBaseKey == sessionState->getAliceBaseKey())
    {
        return true;
    }

    for (SessionState *state: previousStates) {
        if (state->getSessionVersion() == version
                && aliceBaseKey == state->getAliceBaseKey())
        {
            return true;
        }
    }

    return false;
}

SessionState *SessionRecord::getSessionState()
{
    return sessionState;
}

std::vector<SessionState *> SessionRecord::getPreviousSessionStates()
{
    return previousStates;
}

bool SessionRecord::isFresh() const
{
    return fresh;
}

void SessionRecord::promoteState(SessionState *promotedState)
{
    previousStates.insert(previousStates.begin(), promotedState);
    sessionState = promotedState;
    if (previousStates.size() > ARCHIVED_STATES_MAX_LENGTH) {
        previousStates.pop_back();
    }
}

void SessionRecord::archiveCurrentState()
{
    promoteState(new SessionState());
}

void SessionRecord::setState(SessionState *sessionState)
{
    this->sessionState = sessionState;
}

ByteArray SessionRecord::serialize() const
{
    textsecure::RecordStructure record;
    record.mutable_currentsession()->CopyFrom(sessionState->getStructure());

    for (SessionState *previousState: previousStates) {
        record.add_previoussessions()->CopyFrom(previousState->getStructure());
    }

    ::std::string serialized = record.SerializeAsString();
    return ByteArray(serialized.data(), serialized.length());
}
