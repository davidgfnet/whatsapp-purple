#ifndef WHISPEREXCEPTION_H
#define WHISPEREXCEPTION_H

#include <exception>
#include <string>

class WhisperException : public std::exception
{
public:
    WhisperException(const std::string &type, const std::string &error = "Unknown error") throw() {
        _error = error;
        _type = type;
    }
    std::string errorType() const {
        return _type;
    }
    std::string errorMessage() const {
        return _error;
    }
    WhisperException(const WhisperException &source) {
        _error = source.errorMessage();
    }
    virtual ~WhisperException() throw() {}

    void raise() const { throw *this; }
    WhisperException *clone() const { return new WhisperException(*this); }

protected:
    std::string _error;
    std::string _type;
};

#endif // WHISPEREXCEPTION_H
