#ifndef WHISPERMESSAGE_H
#define WHISPERMESSAGE_H

#include "ciphertextmessage.h"
#include "djbec.h"
#include "identitykey.h"
#include "byteutil.h"

class WhisperMessage : public CiphertextMessage
{
public:
    static const int MAC_LENGTH = 8;

    WhisperMessage();
    virtual ~WhisperMessage() {}
    WhisperMessage(const ByteArray &serialized);
    WhisperMessage(int messageVersion, const ByteArray &macKey, const DjbECPublicKey &senderRatchetKey,
                   uint counter, uint previousCounter, const ByteArray &ciphertext,
                   const IdentityKey &senderIdentityKey,
                   const IdentityKey &receiverIdentityKey);
    void verifyMac(int messageVersion, const IdentityKey &senderIdentityKey,
                   const IdentityKey &receiverIdentityKey, const ByteArray &macKey) const;

    DjbECPublicKey getSenderRatchetKey() const;
    int getMessageVersion() const;
    uint getCounter() const;
    ByteArray getBody() const;
    ByteArray serialize() const;
    int getType() const;

    static bool isLegacy(const ByteArray &message) {
        return !message.empty() && message.length() >= 1 &&
            ByteUtil::highBitsToInt(message[0]) <= CiphertextMessage::UNSUPPORTED_VERSION;
    }

private:
    ByteArray getMac(int messageVersion,
                      const IdentityKey &senderIdentityKey,
                      const IdentityKey &receiverIdentityKey,
                      const ByteArray &macKey, ByteArray &serialized) const;

    int         messageVersion;
    DjbECPublicKey senderRatchetKey;
    uint        counter;
    uint        previousCounter;
    ByteArray  ciphertext;
    ByteArray  serialized;
};

#endif // WHISPERMESSAGE_H
