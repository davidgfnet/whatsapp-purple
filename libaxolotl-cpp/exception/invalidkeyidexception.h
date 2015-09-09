#ifndef INVALIDKEYIDEXCEPTION_H
#define INVALIDKEYIDEXCEPTION_H

#include "whisperexception.h"

class InvalidKeyIdException : public WhisperException
{
public:
    InvalidKeyIdException(const std::string &error): WhisperException("InvalidKeyIdException", error) {}
};

#endif // INVALIDKEYIDEXCEPTION_H
