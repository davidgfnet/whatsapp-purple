#ifndef LEGACYMESSAGEEXCEPTION_H
#define LEGACYMESSAGEEXCEPTION_H

#include "whisperexception.h"

class LegacyMessageException : public WhisperException
{
public:
    LegacyMessageException(const std::string &error) : WhisperException("LegacyMessageException", error) {}
};

#endif // LEGACYMESSAGEEXCEPTION_H
