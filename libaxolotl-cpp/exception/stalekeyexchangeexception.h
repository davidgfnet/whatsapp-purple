#ifndef STALEKEYEXCHANGEEXCEPTION_H
#define STALEKEYEXCHANGEEXCEPTION_H

#include "whisperexception.h"

class StaleKeyExchangeException : public WhisperException
{
public:
    StaleKeyExchangeException(const std::string &error) : WhisperException("StaleKeyExchangeException", error) {}
};

#endif // STALEKEYEXCHANGEEXCEPTION_H
