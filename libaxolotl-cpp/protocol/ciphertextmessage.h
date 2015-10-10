#ifndef CIPHERTEXTMESSAGE_H
#define CIPHERTEXTMESSAGE_H

#include "byteutil.h"

class CiphertextMessage
{
public:
    static const int UNSUPPORTED_VERSION;
    static const int CURRENT_VERSION;

    static const int WHISPER_TYPE;
    static const int PREKEY_TYPE;
    static const int SENDERKEY_TYPE;
    static const int SENDERKEY_DISTRIBUTION_TYPE;

    // This should be the worst case (worse than V2).  So not always accurate, but good enough for padding.
    static const int ENCRYPTED_MESSAGE_OVERHEAD;

    virtual ByteArray serialize() const = 0;
    virtual int getType() const = 0;
    virtual ~CiphertextMessage() {}
};

#endif // CIPHERTEXTMESSAGE_H
