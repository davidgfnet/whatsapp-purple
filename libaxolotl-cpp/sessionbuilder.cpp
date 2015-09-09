#include "sessionbuilder.h"

#include "untrustedidentityexception.h"
#include "invalidkeyidexception.h"
#include "invalidkeyexception.h"
#include "invalidmessageexception.h"
#include "stalekeyexchangeexception.h"

#include "ratchet/bobaxolotlparameters.h"
#include "ratchet/ratchetingsession.h"
#include "protocol/ciphertextmessage.h"
#include "util/medium.h"
#include "util/keyhelper.h"

#include <iostream>
#include <memory>

SessionBuilder::SessionBuilder()
{

}

SessionBuilder::SessionBuilder(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore, std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore, uint64_t recipientId, int deviceId)
{
    init(sessionStore, preKeyStore, signedPreKeyStore, identityKeyStore, recipientId, deviceId);
}

SessionBuilder::SessionBuilder(std::shared_ptr<AxolotlStore> store, uint64_t recipientId, int deviceId)
{
    init(std::dynamic_pointer_cast<SessionStore>(store),
         std::dynamic_pointer_cast<PreKeyStore>(store),
         std::dynamic_pointer_cast<SignedPreKeyStore>(store),
         std::dynamic_pointer_cast<IdentityKeyStore>(store),
         recipientId, deviceId);
}

void SessionBuilder::init(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore, std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore, uint64_t recipientId, int deviceId)
{
    this->sessionStore      = sessionStore;
    this->preKeyStore       = preKeyStore;
    this->signedPreKeyStore = signedPreKeyStore;
    this->identityKeyStore  = identityKeyStore;
    this->recipientId       = recipientId;
    this->deviceId          = deviceId;
}

uint64_t SessionBuilder::process(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message)
{
    int         messageVersion   = message->getMessageVersion();
    IdentityKey theirIdentityKey = message->getIdentityKey();

    uint64_t unsignedPreKeyId;

    if (!identityKeyStore->isTrustedIdentity(recipientId, theirIdentityKey)) {
        throw UntrustedIdentityException("Untrusted identity: " + std::to_string(recipientId));
    }

	std::cerr << "Got message version " << messageVersion << std::endl;
    switch (messageVersion) {
        case 2:  unsignedPreKeyId = processV2(sessionRecord, message); break;
        case 3:  unsignedPreKeyId = processV3(sessionRecord, message); break;
        default: throw InvalidMessageException("Unknown version: " + messageVersion);
    }

    identityKeyStore->saveIdentity(recipientId, theirIdentityKey);
    return unsignedPreKeyId;
}

uint64_t SessionBuilder::processV3(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message)
{
    if (sessionRecord->hasSessionState(message->getMessageVersion(), message->getBaseKey().serialize())) {
        return -1;
    }

    ECKeyPair ourSignedPreKey = signedPreKeyStore->loadSignedPreKey(message->getSignedPreKeyId()).getKeyPair();

    BobAxolotlParameters parameters;

    parameters.setTheirBaseKey(message->getBaseKey());
    parameters.setTheirIdentityKey(message->getIdentityKey());
    parameters.setOurIdentityKey(identityKeyStore->getIdentityKeyPair());
    parameters.setOurSignedPreKey(ourSignedPreKey);
    parameters.setOurRatchetKey(ourSignedPreKey);

    if (message->getPreKeyId() >= 0) {
        parameters.setOurOneTimePreKey(preKeyStore->loadPreKey(message->getPreKeyId()).getKeyPair());
    } else {
        //parameters.setOurOneTimePreKey(NULL);
    }

    if (!sessionRecord->isFresh()) sessionRecord->archiveCurrentState();

    RatchetingSession::initializeSession(sessionRecord->getSessionState(), message->getMessageVersion(), parameters);

    sessionRecord->getSessionState()->setLocalRegistrationId(identityKeyStore->getLocalRegistrationId());
    sessionRecord->getSessionState()->setRemoteRegistrationId(message->getRegistrationId());
    sessionRecord->getSessionState()->setAliceBaseKey(message->getBaseKey().serialize());

    if (message->getPreKeyId() != Medium::MAX_VALUE) {
        return message->getPreKeyId();
    } else {
        return -1;
    }
}

uint64_t SessionBuilder::processV2(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message)
{
    if (message->getPreKeyId() < 0) {
        throw InvalidKeyIdException("V2 message requires one time prekey id!");
    }

    if (!preKeyStore->containsPreKey(message->getPreKeyId()) &&
        sessionStore->containsSession(recipientId, deviceId))
    {
        return -1;
    }

    ECKeyPair ourPreKey = preKeyStore->loadPreKey(message->getPreKeyId()).getKeyPair();

    BobAxolotlParameters parameters;

    parameters.setOurIdentityKey(identityKeyStore->getIdentityKeyPair());
    parameters.setOurSignedPreKey(ourPreKey);
    parameters.setOurRatchetKey(ourPreKey);
    //parameters.setOurOneTimePreKey(NULL);
    parameters.setTheirIdentityKey(message->getIdentityKey());
    parameters.setTheirBaseKey(message->getBaseKey());

    if (!sessionRecord->isFresh()) sessionRecord->archiveCurrentState();

    RatchetingSession::initializeSession(sessionRecord->getSessionState(), message->getMessageVersion(), parameters);

    sessionRecord->getSessionState()->setLocalRegistrationId(identityKeyStore->getLocalRegistrationId());
    sessionRecord->getSessionState()->setRemoteRegistrationId(message->getRegistrationId());
    sessionRecord->getSessionState()->setAliceBaseKey(message->getBaseKey().serialize());

    if (message->getPreKeyId() != Medium::MAX_VALUE) {
        return message->getPreKeyId();
    } else {
        return -1;
    }
}

void SessionBuilder::process(const PreKeyBundle &preKey)
{
    if (!identityKeyStore->isTrustedIdentity(recipientId, preKey.getIdentityKey())) {
        throw UntrustedIdentityException("Untrusted identity: " + std::to_string(recipientId));
    }

    if (!preKey.getSignedPreKey().serialize().empty() &&
        !Curve::verifySignature(preKey.getSignedPreKey(),
                                preKey.getIdentityKey().getPublicKey().serialize(),
                                preKey.getSignedPreKeySignature()))
    {
        std::cerr << ByteUtil::toHex(preKey.getIdentityKey().getPublicKey().serialize());
        std::cerr << ByteUtil::toHex(preKey.getSignedPreKey().serialize());
        std::cerr << ByteUtil::toHex(preKey.getSignedPreKeySignature());
        throw InvalidKeyException("Invalid signature on device key!");
    }

    if (preKey.getSignedPreKey().serialize().empty() && preKey.getPreKey().serialize().empty()) {
        throw InvalidKeyException("Both signed and unsigned prekeys are absent!");
    }

    bool           supportsV3           = !preKey.getSignedPreKey().serialize().empty();
    SessionRecord *sessionRecord        = sessionStore->loadSession(recipientId, deviceId);
    ECKeyPair      ourBaseKey           = Curve::generateKeyPair();
    DjbECPublicKey theirSignedPreKey    = supportsV3 ? preKey.getSignedPreKey() : preKey.getPreKey();
    DjbECPublicKey theirOneTimePreKey   = preKey.getPreKey();
    int            theirOneTimePreKeyId = theirOneTimePreKey.serialize().empty() ? -1 : preKey.getPreKeyId();

    AliceAxolotlParameters parameters;

    parameters.setOurBaseKey(ourBaseKey);
    parameters.setOurIdentityKey(identityKeyStore->getIdentityKeyPair());
    parameters.setTheirIdentityKey(preKey.getIdentityKey());
    parameters.setTheirSignedPreKey(theirSignedPreKey);
    parameters.setTheirRatchetKey(theirSignedPreKey);
    if (supportsV3) {
        parameters.setTheirOneTimePreKey(theirOneTimePreKey);
    }

    if (!sessionRecord->isFresh()) sessionRecord->archiveCurrentState();

    RatchetingSession::initializeSession(sessionRecord->getSessionState(),
                                         supportsV3 ? 3 : 2,
                                         parameters);

    sessionRecord->getSessionState()->setUnacknowledgedPreKeyMessage(theirOneTimePreKeyId, preKey.getSignedPreKeyId(), ourBaseKey.getPublicKey());
    sessionRecord->getSessionState()->setLocalRegistrationId(identityKeyStore->getLocalRegistrationId());
    sessionRecord->getSessionState()->setRemoteRegistrationId(preKey.getRegistrationId());
    sessionRecord->getSessionState()->setAliceBaseKey(ourBaseKey.getPublicKey().serialize());

    sessionStore->storeSession(recipientId, deviceId, sessionRecord);
    identityKeyStore->saveIdentity(recipientId, preKey.getIdentityKey());
}

KeyExchangeMessage SessionBuilder::process(std::shared_ptr<KeyExchangeMessage> message)
{
    if (!identityKeyStore->isTrustedIdentity(recipientId, message->getIdentityKey())) {
        throw UntrustedIdentityException("Untrusted identity: %1" + std::to_string(recipientId));
    }

    KeyExchangeMessage responseMessage;

    if (message->isInitiate()) responseMessage = processInitiate(message);
    else                       processResponse(message);

    return responseMessage;
}

KeyExchangeMessage SessionBuilder::process()
{
    int             sequence         = KeyHelper::getRandomFFFF();
    int             flags            = KeyExchangeMessage::INITIATE_FLAG;
    ECKeyPair       baseKey          = Curve::generateKeyPair();
    ECKeyPair       ratchetKey       = Curve::generateKeyPair();
    IdentityKeyPair identityKey      = identityKeyStore->getIdentityKeyPair();
    ByteArray       baseKeySignature = Curve::calculateSignature(identityKey.getPrivateKey(), baseKey.getPublicKey().serialize());
    SessionRecord  *sessionRecord    = sessionStore->loadSession(recipientId, deviceId);

    sessionRecord->getSessionState()->setPendingKeyExchange(sequence, baseKey, ratchetKey, identityKey);
    sessionStore->storeSession(recipientId, deviceId, sessionRecord);

    return KeyExchangeMessage(2, sequence, flags, baseKey.getPublicKey(), baseKeySignature,
                              ratchetKey.getPublicKey(), identityKey.getPublicKey());
}

KeyExchangeMessage SessionBuilder::processInitiate(std::shared_ptr<KeyExchangeMessage> message)
{
    int            flags         = KeyExchangeMessage::RESPONSE_FLAG;
    SessionRecord *sessionRecord = sessionStore->loadSession(recipientId, deviceId);

    if (message->getVersion() >= 3 &&
        !Curve::verifySignature(message->getIdentityKey().getPublicKey(),
                                message->getBaseKey().serialize(),
                                message->getBaseKeySignature()))
    {
        throw InvalidKeyException("Bad signature!");
    }

    SymmetricAxolotlParameters parameters;

    if (!sessionRecord->getSessionState()->hasPendingKeyExchange()) {
        parameters.setOurIdentityKey(identityKeyStore->getIdentityKeyPair());
        parameters.setOurBaseKey(Curve::generateKeyPair());
        parameters.setOurRatchetKey(Curve::generateKeyPair());
    } else {
        parameters.setOurIdentityKey(sessionRecord->getSessionState()->getPendingKeyExchangeIdentityKey());
        parameters.setOurBaseKey(sessionRecord->getSessionState()->getPendingKeyExchangeBaseKey());
        parameters.setOurRatchetKey(sessionRecord->getSessionState()->getPendingKeyExchangeRatchetKey());
        flags |= KeyExchangeMessage::SIMULTAENOUS_INITIATE_FLAG;
    }

    parameters.setTheirBaseKey(message->getBaseKey());
    parameters.setTheirRatchetKey(message->getRatchetKey());
    parameters.setTheirIdentityKey(message->getIdentityKey());

    if (!sessionRecord->isFresh()) sessionRecord->archiveCurrentState();

    RatchetingSession::initializeSession(sessionRecord->getSessionState(),
                                         std::min(message->getMaxVersion(), CiphertextMessage::CURRENT_VERSION),
                                         parameters);

    sessionStore->storeSession(recipientId, deviceId, sessionRecord);
    identityKeyStore->saveIdentity(recipientId, message->getIdentityKey());

    ByteArray baseKeySignature = Curve::calculateSignature(parameters.getOurIdentityKey().getPrivateKey(),
                                                            parameters.getOurBaseKey().getPublicKey().serialize());

    return KeyExchangeMessage(sessionRecord->getSessionState()->getSessionVersion(),
                              message->getSequence(), flags,
                              parameters.getOurBaseKey().getPublicKey(),
                              baseKeySignature, parameters.getOurRatchetKey().getPublicKey(),
                              parameters.getOurIdentityKey().getPublicKey());
}

void SessionBuilder::processResponse(std::shared_ptr<KeyExchangeMessage> message)
{
    SessionRecord *sessionRecord                  = sessionStore->loadSession(recipientId, deviceId);
    SessionState  *sessionState                   = sessionRecord->getSessionState();
    bool           hasPendingKeyExchange          = sessionState->hasPendingKeyExchange();
    bool           isSimultaneousInitiateResponse = message->isResponseForSimultaneousInitiate();

    if (!hasPendingKeyExchange || sessionState->getPendingKeyExchangeSequence() != message->getSequence()) {
        if (!isSimultaneousInitiateResponse) throw StaleKeyExchangeException("");
        else                                 return;
    }

    SymmetricAxolotlParameters parameters;

    parameters.setOurBaseKey(sessionRecord->getSessionState()->getPendingKeyExchangeBaseKey());
    parameters.setOurRatchetKey(sessionRecord->getSessionState()->getPendingKeyExchangeRatchetKey());
    parameters.setOurIdentityKey(sessionRecord->getSessionState()->getPendingKeyExchangeIdentityKey());
    parameters.setTheirBaseKey(message->getBaseKey());
    parameters.setTheirRatchetKey(message->getRatchetKey());
    parameters.setTheirIdentityKey(message->getIdentityKey());

    if (!sessionRecord->isFresh()) sessionRecord->archiveCurrentState();

    RatchetingSession::initializeSession(sessionRecord->getSessionState(),
                                         std::min(message->getMaxVersion(), CiphertextMessage::CURRENT_VERSION),
                                         parameters);

    if (sessionRecord->getSessionState()->getSessionVersion() >= 3 &&
        !Curve::verifySignature(message->getIdentityKey().getPublicKey(),
                                message->getBaseKey().serialize(),
                                message->getBaseKeySignature()))
    {
        throw InvalidKeyException("Base key signature doesn't match!");
    }

    sessionStore->storeSession(recipientId, deviceId, sessionRecord);
    identityKeyStore->saveIdentity(recipientId, message->getIdentityKey());
}
