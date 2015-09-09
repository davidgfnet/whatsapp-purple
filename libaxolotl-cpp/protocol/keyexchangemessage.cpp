#include "keyexchangemessage.h"
#include "ciphertextmessage.h"
#include "WhisperTextProtocol.pb.h"
#include "curve.h"

#include "legacymessageexception.h"
#include "invalidversionexception.h"
#include "invalidmessageexception.h"

KeyExchangeMessage::KeyExchangeMessage()
{

}

KeyExchangeMessage::KeyExchangeMessage(int messageVersion, int sequence, int flags, const DjbECPublicKey &baseKey, const ByteArray &baseKeySignature, const DjbECPublicKey &ratchetKey, const IdentityKey &identityKey)
{
    this->supportedVersion = CiphertextMessage::CURRENT_VERSION;
    this->version          = messageVersion;
    this->sequence         = sequence;
    this->flags            = flags;
    this->baseKey          = baseKey;
    this->baseKeySignature = baseKeySignature;
    this->ratchetKey       = ratchetKey;
    this->identityKey      = identityKey;

    textsecure::KeyExchangeMessage message;
    message.set_id((sequence << 5) | flags);
    message.set_basekey(baseKey.serialize().c_str());
    message.set_ratchetkey(ratchetKey.serialize().c_str());
    message.set_identitykey(identityKey.serialize().c_str());

    if (messageVersion >= 3) {
        message.set_basekeysignature(baseKeySignature.c_str());
    }

    ::std::string serializedMessage = message.SerializeAsString();
    this->serialized = ByteArray(1, ByteUtil::intsToByteHighAndLow(this->version, this->supportedVersion));
	this->serialized += ByteArray(serializedMessage.data(), serializedMessage.length());
}

KeyExchangeMessage::KeyExchangeMessage(const ByteArray &serialized)
{
    std::vector<ByteArray> parts = ByteUtil::split(serialized, 1, serialized.size() - 1);
    this->version           = ByteUtil::highBitsToInt(parts[0][0]);
    this->supportedVersion  = ByteUtil::lowBitsToInt(parts[0][0]);

    if (this->version <= CiphertextMessage::UNSUPPORTED_VERSION) {
        throw LegacyMessageException("Unsupported legacy version: " + std::to_string(this->version));
    }

    if (this->version > CiphertextMessage::CURRENT_VERSION) {
        throw InvalidVersionException("Unknown version: " + std::to_string(this->version));
    }

    textsecure::KeyExchangeMessage message;
    message.ParseFromArray(parts[1].c_str(), parts[1].size());

    if (!message.has_id()          || !message.has_basekey()     ||
        !message.has_ratchetkey()  || !message.has_identitykey() ||
        (this->version >=3 && !message.has_basekeysignature()))
    {
        throw InvalidMessageException("Some required fields missing!");
    }

    this->sequence         = message.id() >> 5;
    this->flags            = message.id() & 0x1f;
    this->serialized       = serialized;
    ::std::string messagebasekey = message.basekey();
    this->baseKey          = Curve::decodePoint(ByteArray(messagebasekey.data(), messagebasekey.length()), 0);
    ::std::string messagebasekeysignature = message.basekeysignature();
    this->baseKeySignature = ByteArray(messagebasekeysignature.data(), messagebasekeysignature.length());
    ::std::string messageratchetkey = message.ratchetkey();
    this->ratchetKey       = Curve::decodePoint(ByteArray(messageratchetkey.data(), messageratchetkey.length()), 0);
    ::std::string messageidentitykey = message.identitykey();
    this->identityKey      = IdentityKey(ByteArray(messageidentitykey.data(), messageidentitykey.length()), 0);
}

int KeyExchangeMessage::getVersion() const
{
    return version;
}

DjbECPublicKey KeyExchangeMessage::getBaseKey() const
{
    return baseKey;
}

ByteArray KeyExchangeMessage::getBaseKeySignature() const
{
    return baseKeySignature;
}

DjbECPublicKey KeyExchangeMessage::getRatchetKey() const
{
    return ratchetKey;
}

IdentityKey KeyExchangeMessage::getIdentityKey() const
{
    return identityKey;
}

bool KeyExchangeMessage::hasIdentityKey() const
{
    return true;
}

int KeyExchangeMessage::getMaxVersion() const
{
    return supportedVersion;
}

bool KeyExchangeMessage::isResponse() const
{
    return (flags & RESPONSE_FLAG) != 0;
}

bool KeyExchangeMessage::isInitiate() const
{
    return (flags & INITIATE_FLAG) != 0;
}

bool KeyExchangeMessage::isResponseForSimultaneousInitiate() const
{
    return (flags & SIMULTAENOUS_INITIATE_FLAG) != 0;
}

int KeyExchangeMessage::getFlags() const
{
    return flags;
}

int KeyExchangeMessage::getSequence() const
{
    return sequence;
}

ByteArray KeyExchangeMessage::serialize() const
{
    return serialized;
}
