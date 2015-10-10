#ifndef PREKEYWHISPERMESSAGE_H
#define PREKEYWHISPERMESSAGE_H

#include <memory>

#include "ciphertextmessage.h"
#include "djbec.h"
#include "identitykey.h"
#include "whispermessage.h"
#include "byteutil.h"

class PreKeyWhisperMessage : public CiphertextMessage
{
public:
    PreKeyWhisperMessage(const ByteArray &serialized);
    PreKeyWhisperMessage(int messageVersion, ulong registrationId, ulong preKeyId,
                         ulong signedPreKeyId, const DjbECPublicKey &baseKey, const IdentityKey &identityKey,
                         std::shared_ptr<WhisperMessage> message);
    virtual ~PreKeyWhisperMessage() {}

    int getMessageVersion() const;
    IdentityKey getIdentityKey() const;
    ulong getRegistrationId() const;
    ulong getPreKeyId() const;
    ulong getSignedPreKeyId() const;
    DjbECPublicKey getBaseKey() const;
    std::shared_ptr<WhisperMessage> getWhisperMessage();
    ByteArray serialize() const;
    int getType() const;

private:
    int               version;
    ulong             registrationId;
    ulong             preKeyId;
    ulong             signedPreKeyId;
    DjbECPublicKey    baseKey;
    IdentityKey       identityKey;
    std::shared_ptr<WhisperMessage> message;
    ByteArray        serialized;
};

#endif // PREKEYWHISPERMESSAGE_H
