#include "senderkeymessage.h"

#include <iostream>
#include "legacymessageexception.h"
#include "invalidmessageexception.h"
#include "WhisperTextProtocol.pb.h"
#include "curve.h"
#include "invalidkeyexception.h"

SenderKeyMessage::SenderKeyMessage(const ByteArray &serialized)
{
    std::vector<ByteArray> messageParts = ByteUtil::split(serialized, 1, serialized.size() - 1 - SIGNATURE_LENGTH, SIGNATURE_LENGTH);
    uint8_t     version             = messageParts[0][0];
    ByteArray message             = messageParts[1];

    if (ByteUtil::highBitsToInt(version) < 3) {
        throw LegacyMessageException("Legacy message: " + ByteUtil::highBitsToInt(version));
    }

    if (ByteUtil::highBitsToInt(version) > CURRENT_VERSION) {
        throw InvalidMessageException("Unknown version: " + ByteUtil::highBitsToInt(version));
    }

    textsecure::SenderKeyMessage senderKeyMessage;
    senderKeyMessage.ParseFromArray(message.c_str(), message.size());

    if (!senderKeyMessage.has_id() ||
        !senderKeyMessage.has_iteration() ||
        !senderKeyMessage.has_ciphertext())
    {
        std::cerr << "has_id" << senderKeyMessage.has_id();
        std::cerr << "has_iteration" << senderKeyMessage.has_iteration();
        std::cerr << "has_ciphertext" << senderKeyMessage.has_ciphertext();
        throw InvalidMessageException("Incomplete message.");
    }

    this->serialized     = serialized;
    this->messageVersion = ByteUtil::highBitsToInt(version);
    this->keyId          = senderKeyMessage.id();
    this->iteration      = senderKeyMessage.iteration();
    ::std::string senderKeyMessageCiphertext = senderKeyMessage.ciphertext();
    this->ciphertext     = ByteArray(senderKeyMessageCiphertext.data(), senderKeyMessageCiphertext.length());
}

SenderKeyMessage::SenderKeyMessage(uint64_t keyId, int iteration, const ByteArray &ciphertext, const DjbECPrivateKey &signatureKey)
{
    textsecure::SenderKeyMessage senderKeyMessage;
    senderKeyMessage.set_id(keyId);
    senderKeyMessage.set_iteration(iteration);
    senderKeyMessage.set_ciphertext(ciphertext.c_str());
    ::std::string serializedMessage = senderKeyMessage.SerializeAsString();
    ByteArray message(serializedMessage.data(), serializedMessage.length());
    message = ByteArray(1, ByteUtil::intsToByteHighAndLow(CURRENT_VERSION, CURRENT_VERSION)) + message;
    message += getSignature(signatureKey, message);

    this->serialized       = message;
    this->messageVersion   = CURRENT_VERSION;
    this->keyId            = keyId;
    this->iteration        = iteration;
    this->ciphertext       = ciphertext;
}

void SenderKeyMessage::verifySignature(const DjbECPublicKey &signatureKey)
{
    try {
      std::vector<ByteArray> parts = ByteUtil::split(serialized, serialized.size() - SIGNATURE_LENGTH, SIGNATURE_LENGTH);

      if (!Curve::verifySignature(signatureKey, parts[0], parts[1])) {
          throw InvalidMessageException("Invalid signature!");
      }

    } catch (const InvalidKeyException &e) {
        throw InvalidMessageException(__PRETTY_FUNCTION__, {e});
    }
}

uint64_t SenderKeyMessage::getKeyId() const
{
    return keyId;
}

int SenderKeyMessage::getIteration() const
{
    return iteration;
}

ByteArray SenderKeyMessage::getCipherText() const
{
    return ciphertext;
}

ByteArray SenderKeyMessage::serialize() const
{
    return serialized;
}

int SenderKeyMessage::getType() const
{
    return CiphertextMessage::SENDERKEY_TYPE;
}

ByteArray SenderKeyMessage::getSignature(const DjbECPrivateKey &signatureKey, const ByteArray &serialized)
{
    return Curve::calculateSignature(signatureKey, serialized);
}
