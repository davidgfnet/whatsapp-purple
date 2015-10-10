#ifndef UNTRUSTEDIDENTITYEXCEPTION_H
#define UNTRUSTEDIDENTITYEXCEPTION_H

#include "whisperexception.h"

class UntrustedIdentityException : public WhisperException
{
public:
    UntrustedIdentityException(const std::string &error) : WhisperException("UntrustedIdentityException", error) {}
};

#endif // UNTRUSTEDIDENTITYEXCEPTION_H
