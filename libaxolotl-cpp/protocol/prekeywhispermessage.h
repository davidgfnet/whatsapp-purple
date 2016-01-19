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
    PreKeyWhisperMessage(int messageVersion, uint64_t registrationId, uint64_t preKeyId,
                         uint64_t signedPreKeyId, const DjbECPublicKey &baseKey, const IdentityKey &identityKey,
                         std::shared_ptr<WhisperMessage> message);
    virtual ~PreKeyWhisperMessage() {}

    int getMessageVersion() const;
    IdentityKey getIdentityKey() const;
    uint64_t getRegistrationId() const;
    uint64_t getPreKeyId() const;
    uint64_t getSignedPreKeyId() const;
    DjbECPublicKey getBaseKey() const;
    std::shared_ptr<WhisperMessage> getWhisperMessage();
    ByteArray serialize() const;
    int getType() const;

private:
    int               version;
    uint64_t          registrationId;
    uint64_t          preKeyId;
    uint64_t          signedPreKeyId;
    DjbECPublicKey    baseKey;
    IdentityKey       identityKey;
    std::shared_ptr<WhisperMessage> message;
    ByteArray        serialized;
};

#endif // PREKEYWHISPERMESSAGE_H
