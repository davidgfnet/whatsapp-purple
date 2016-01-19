#include "prekeywhispermessage.h"
#include "invalidkeyexception.h"
#include "legacymessageexception.h"
#include "invalidmessageexception.h"
#include "invalidversionexception.h"

#include "byteutil.h"
#include "curve.h"
#include "WhisperTextProtocol.pb.h"
#include <iostream>

PreKeyWhisperMessage::PreKeyWhisperMessage(const ByteArray &serialized)
{
    try {
        this->version = ByteUtil::highBitsToInt(serialized[0]);

        if (this->version > CiphertextMessage::CURRENT_VERSION) {
            throw InvalidVersionException("Unknown version: " + this->version);
        }
        textsecure::PreKeyWhisperMessage preKeyWhisperMessage;
        ByteArray serializedMessage = serialized.substr(1);
        preKeyWhisperMessage.ParseFromArray(serializedMessage.c_str(), serializedMessage.size());

        if ((version == 2 && !preKeyWhisperMessage.has_prekeyid())        ||
            (version == 3 && !preKeyWhisperMessage.has_signedprekeyid())  ||
            !preKeyWhisperMessage.has_basekey()                           ||
            !preKeyWhisperMessage.has_identitykey()                       ||
            !preKeyWhisperMessage.has_message())
        {
            std::cerr << "version:" << version << std::endl;
            std::cerr << "has_prekeyid:" << preKeyWhisperMessage.has_prekeyid() << std::endl;
            std::cerr << "has_signedprekeyid:" << preKeyWhisperMessage.has_signedprekeyid() << std::endl;
            std::cerr << "has_basekey:" << preKeyWhisperMessage.has_basekey() << std::endl;
            std::cerr << "has_identitykey:" << preKeyWhisperMessage.has_identitykey() << std::endl;
            std::cerr << "has_message:" << preKeyWhisperMessage.has_message() << std::endl;
            throw InvalidMessageException("Incomplete message.");
        }

        this->serialized     = serialized;
        this->registrationId = preKeyWhisperMessage.registrationid();
        this->preKeyId       = preKeyWhisperMessage.has_prekeyid() ? preKeyWhisperMessage.prekeyid() : -1;
        this->signedPreKeyId = preKeyWhisperMessage.has_signedprekeyid() ? preKeyWhisperMessage.signedprekeyid() : -1;
        ::std::string basekey = preKeyWhisperMessage.basekey();
        this->baseKey        = Curve::decodePoint(ByteArray(basekey.data(), basekey.length()), 0);
        ::std::string identitykey = preKeyWhisperMessage.identitykey();
        this->identityKey    = IdentityKey(Curve::decodePoint(ByteArray(identitykey.data(), identitykey.length()), 0));
        ::std::string whisperMessage = preKeyWhisperMessage.message();
        ByteArray whisperMessageSerialized(whisperMessage.data(), whisperMessage.length());
        this->message.reset(new WhisperMessage(whisperMessageSerialized));
    } catch (const InvalidKeyException &e) {
        throw InvalidMessageException(__PRETTY_FUNCTION__, {e});
    } catch (const LegacyMessageException &e) {
        throw InvalidMessageException(__PRETTY_FUNCTION__, {e});
    }
}

PreKeyWhisperMessage::PreKeyWhisperMessage(int messageVersion, uint64_t registrationId, uint64_t preKeyId, uint64_t signedPreKeyId, const DjbECPublicKey &baseKey, const IdentityKey &identityKey, std::shared_ptr<WhisperMessage> message)
{
    this->version        = messageVersion;
    this->registrationId = registrationId;
    this->preKeyId       = preKeyId;
    this->signedPreKeyId = signedPreKeyId;
    this->baseKey        = baseKey;
    this->identityKey    = identityKey;
    this->message        = message;

    textsecure::PreKeyWhisperMessage preKeyWhisperMessage;
    preKeyWhisperMessage.set_signedprekeyid(signedPreKeyId);
    ByteArray basekey = baseKey.serialize();
    preKeyWhisperMessage.set_basekey(basekey.c_str(), basekey.size());
    ByteArray identitykey = identityKey.serialize();
    preKeyWhisperMessage.set_identitykey(identitykey.c_str(), identitykey.size());
    ByteArray bytemessage = message->serialize();
    preKeyWhisperMessage.set_message(bytemessage.c_str(), bytemessage.size());
    preKeyWhisperMessage.set_registrationid(registrationId);

    if (preKeyId >= 0) {
        preKeyWhisperMessage.set_prekeyid(preKeyId);
    }

    ::std::string serializedmessage = preKeyWhisperMessage.SerializeAsString();
    ByteArray messageBytes(serializedmessage.data(), serializedmessage.length());

    this->serialized = messageBytes;
    this->serialized = ByteArray(1, ByteUtil::intsToByteHighAndLow(this->version, CURRENT_VERSION)) + this->serialized;
}

int PreKeyWhisperMessage::getMessageVersion() const
{
    return version;
}

IdentityKey PreKeyWhisperMessage::getIdentityKey() const
{
    return identityKey;
}

uint64_t PreKeyWhisperMessage::getRegistrationId() const
{
    return registrationId;
}

uint64_t PreKeyWhisperMessage::getPreKeyId() const
{
    return preKeyId;
}

uint64_t PreKeyWhisperMessage::getSignedPreKeyId() const
{
    return signedPreKeyId;
}

DjbECPublicKey PreKeyWhisperMessage::getBaseKey() const
{
    return baseKey;
}

std::shared_ptr<WhisperMessage> PreKeyWhisperMessage::getWhisperMessage()
{
    return message;
}

ByteArray PreKeyWhisperMessage::serialize() const
{
    return serialized;
}

int PreKeyWhisperMessage::getType() const
{
    return CiphertextMessage::PREKEY_TYPE;
}

