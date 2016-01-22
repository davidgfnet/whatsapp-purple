#include "whispermessage.h"
#include "invalidkeyexception.h"
#include "invalidmessageexception.h"
#include "legacymessageexception.h"
#include "WhisperTextProtocol.pb.h"
#include "curve.h"

#include <iostream>

void HMAC_SHA256(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest);

WhisperMessage::WhisperMessage()
{

}

WhisperMessage::WhisperMessage(const ByteArray &serialized)
{
    try {
        uint8_t     version      = serialized[0];
        ByteArray   message   = serialized.substr(1, serialized.size() - MAC_LENGTH - 1);
        ByteArray   mac       = serialized.substr(serialized.size() - MAC_LENGTH);

        if (ByteUtil::highBitsToInt(version) <= CiphertextMessage::UNSUPPORTED_VERSION) {
            throw LegacyMessageException("Legacy message: " + ByteUtil::highBitsToInt(version));
        }

        if (ByteUtil::highBitsToInt(version) > CURRENT_VERSION) {
            throw InvalidMessageException("Unknown version: " + ByteUtil::highBitsToInt(version));
        }

        textsecure::WhisperMessage whisperMessage;
        whisperMessage.ParsePartialFromArray(message.c_str(), message.size());

        if (!whisperMessage.has_ciphertext() ||
            !whisperMessage.has_counter() ||
            !whisperMessage.has_ratchetkey())
        {
            throw InvalidMessageException("Incomplete message.");
        }

        this->serialized       = serialized;
        ::std::string whisperratchetkey = whisperMessage.ratchetkey();
        ByteArray whisperratchetkeybytes(whisperratchetkey.data(), whisperratchetkey.length());
        this->senderRatchetKey = Curve::decodePoint(whisperratchetkeybytes, 0);
        this->messageVersion   = ByteUtil::highBitsToInt(version);
        this->counter          = whisperMessage.counter();
        this->previousCounter  = whisperMessage.previouscounter();
        ::std::string whisperciphertext = whisperMessage.ciphertext();
        this->ciphertext       = ByteArray(whisperciphertext.data(), whisperciphertext.length());
    } catch (const InvalidKeyException &e) {
        throw InvalidMessageException(__PRETTY_FUNCTION__, {e});
    }
}

WhisperMessage::WhisperMessage(int messageVersion, const ByteArray &macKey, const DjbECPublicKey &senderRatchetKey, unsigned counter, unsigned previousCounter, const ByteArray &ciphertext, const IdentityKey &senderIdentityKey, const IdentityKey &receiverIdentityKey)
{
    textsecure::WhisperMessage whisperMessage;
    ByteArray ratchetKey = senderRatchetKey.serialize();
    whisperMessage.set_ratchetkey(ratchetKey.c_str(), ratchetKey.size());
    whisperMessage.set_counter(counter);
    whisperMessage.set_previouscounter(previousCounter);
    whisperMessage.set_ciphertext(ciphertext.c_str() ,ciphertext.size());
    ::std::string serializedMessage = whisperMessage.SerializeAsString();
    ByteArray message(serializedMessage.data(), serializedMessage.length());
    message = ByteArray(1, ByteUtil::intsToByteHighAndLow(messageVersion, CURRENT_VERSION)) + message;
    ByteArray mac     = getMac(messageVersion, senderIdentityKey, receiverIdentityKey, macKey, message);

    this->serialized       = message;
    this->serialized.append(mac);
    this->senderRatchetKey = senderRatchetKey;
    this->counter          = counter;
    this->previousCounter  = previousCounter;
    this->ciphertext       = ciphertext;
    this->messageVersion   = messageVersion;
}

DjbECPublicKey WhisperMessage::getSenderRatchetKey() const
{
    return senderRatchetKey;
}

int WhisperMessage::getMessageVersion() const
{
    return messageVersion;
}

unsigned WhisperMessage::getCounter() const
{
    return counter;
}

ByteArray WhisperMessage::getBody() const
{
    return ciphertext;
}

ByteArray WhisperMessage::serialize() const
{
    return serialized;
}

int WhisperMessage::getType() const
{
    return CiphertextMessage::WHISPER_TYPE;
}

ByteArray WhisperMessage::getMac(int messageVersion, const IdentityKey &senderIdentityKey, const IdentityKey &receiverIdentityKey, const ByteArray &macKey, ByteArray &serialized) const
{
    ByteArray data;
    if (messageVersion >= 3) {
        data += senderIdentityKey.getPublicKey().serialize();
        data += receiverIdentityKey.getPublicKey().serialize();
    }

    data += serialized;

    unsigned char out[32];
	HMAC_SHA256((unsigned char*)data.c_str(), data.size(), (unsigned char*)macKey.c_str(), macKey.size(), out);
    return ByteArray((const char*)out, MAC_LENGTH);
}

void WhisperMessage::verifyMac(int messageVersion, const IdentityKey &senderIdentityKey, const IdentityKey &receiverIdentityKey, const ByteArray &macKey) const
{
    std::vector<ByteArray> parts = ByteUtil::split(serialized, serialized.size() - MAC_LENGTH, MAC_LENGTH);
    ByteArray ourMac = getMac(messageVersion, senderIdentityKey, receiverIdentityKey, macKey, parts[0]);
    ByteArray theirMac = parts[1];

    if (ourMac != theirMac) {
       throw InvalidMessageException("Bad Mac!");
    }
}
