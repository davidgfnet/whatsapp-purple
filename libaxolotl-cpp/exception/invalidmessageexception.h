#ifndef INVALIDMESSAGEEXCEPTION_H
#define INVALIDMESSAGEEXCEPTION_H

#include "whisperexception.h"

class InvalidMessageException : public WhisperException
{
public:
    InvalidMessageException(const std::string &error) : WhisperException("InvalidMessageException", error) {}
    InvalidMessageException(const std::string &error, const std::vector<WhisperException> &exceptions) : WhisperException("InvalidMessageException", error) {
        for (const WhisperException &exception: exceptions) {
            _error.append(" ");
            _error.append(exception.errorMessage());
        }
    }
};

#endif // INVALIDMESSAGEEXCEPTION_H
