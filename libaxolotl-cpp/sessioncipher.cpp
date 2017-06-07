#include "sessioncipher.h"
#include "nosessionexception.h"
#include "invalidmessageexception.h"
#include "invalidkeyexception.h"
#include "duplicatemessageexception.h"

#include <iostream>
#include <memory>
#include <cstdlib>
#include "aes.h"

static void ctr128_inc(unsigned char *counter) {
    unsigned int  n=16;
    unsigned char c;

    do {
        --n;
        c = counter[n];
        ++c;
        counter[n] = c;
        if (c) return;
    } while (n);
}

static void ctr128_inc_aligned(unsigned char *counter) {
    size_t *data,c,n;
    const union { long one; char little; } is_endian = {1};

    if (is_endian.little) {
        ctr128_inc(counter);
        return;
    }

    data = (size_t *)counter;
    n = 16/sizeof(size_t);
    do {
        --n;
        c = data[n];
        ++c;
        data[n] = c;
        if (c) return;
    } while (n);
}

SessionCipher::SessionCipher(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore, std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore, uint64_t recipientId, int deviceId)
{
    init(sessionStore, preKeyStore, signedPreKeyStore, identityKeyStore, recipientId, deviceId);
}

SessionCipher::SessionCipher(std::shared_ptr<AxolotlStore> store, uint64_t recipientId, int deviceId)
{
    init(std::dynamic_pointer_cast<SessionStore>(store),
         std::dynamic_pointer_cast<PreKeyStore>(store),
         std::dynamic_pointer_cast<SignedPreKeyStore>(store),
         std::dynamic_pointer_cast<IdentityKeyStore>(store),
         recipientId, deviceId);
}

void SessionCipher::init(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore, std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore, uint64_t recipientId, int deviceId)
{
    this->sessionStore   = sessionStore;
    this->recipientId    = recipientId;
    this->deviceId       = deviceId;
    this->preKeyStore    = preKeyStore;
    this->sessionBuilder = SessionBuilder(sessionStore, preKeyStore, signedPreKeyStore,
                                          identityKeyStore, recipientId, deviceId);
}

std::shared_ptr<CiphertextMessage> SessionCipher::encrypt(const ByteArray &paddedMessage)
{
    std::shared_ptr<CiphertextMessage> result;

    SessionRecord *sessionRecord   = sessionStore->loadSession(recipientId, deviceId);
    SessionState  *sessionState    = sessionRecord->getSessionState();
    ChainKey       chainKey        = sessionState->getSenderChainKey();
    MessageKeys    messageKeys     = chainKey.getMessageKeys();
    DjbECPublicKey senderEphemeral = sessionState->getSenderRatchetKey();
    int            previousCounter = sessionState->getPreviousCounter();
    int            sessionVersion  = sessionState->getSessionVersion();

    ByteArray     ciphertextBody  = getCiphertext(sessionVersion, messageKeys, paddedMessage);
    std::shared_ptr<WhisperMessage> whisperMessage(new WhisperMessage(sessionVersion, messageKeys.getMacKey(),
                                                                     senderEphemeral, chainKey.getIndex(),
                                                                     previousCounter, ciphertextBody,
                                                                     sessionState->getLocalIdentityKey(),
                                                                     sessionState->getRemoteIdentityKey()));

    if (sessionState->hasUnacknowledgedPreKeyMessage()) {
        UnacknowledgedPreKeyMessageItems items = sessionState->getUnacknowledgedPreKeyMessageItems();
        int localRegistrationId = sessionState->getLocalRegistrationId();

        std::shared_ptr<PreKeyWhisperMessage> preKeyWhisperMessage(new PreKeyWhisperMessage(
                                                                      sessionVersion, localRegistrationId, items.getPreKeyId(),
                                                                      items.getSignedPreKeyId(), items.getBaseKey(),
                                                                      sessionState->getLocalIdentityKey(),
                                                                      whisperMessage));
        result = preKeyWhisperMessage;
    }
    else {
        result = whisperMessage;
    }

    sessionState->setSenderChainKey(chainKey.getNextChainKey());
    sessionStore->storeSession(recipientId, deviceId, sessionRecord);

    return result;
}

ByteArray SessionCipher::decrypt(std::shared_ptr<PreKeyWhisperMessage> ciphertext)
{
    SessionRecord    *sessionRecord    = sessionStore->loadSession(recipientId, deviceId);
    uint64_t         unsignedPreKeyId  = sessionBuilder.process(sessionRecord, ciphertext);
    ByteArray        plaintext         = decrypt(sessionRecord, ciphertext->getWhisperMessage());

    sessionStore->storeSession(recipientId, deviceId, sessionRecord);

    if (unsignedPreKeyId != -1) {
        //preKeyStore->removePreKey(unsignedPreKeyId);
    }

    return plaintext;
}

ByteArray SessionCipher::decrypt(std::shared_ptr<WhisperMessage> ciphertext)
{
    if (!sessionStore->containsSession(recipientId, deviceId)) {
        throw NoSessionException("No session for: " + std::to_string(recipientId) + "," + std::to_string(deviceId));
    }

    SessionRecord *sessionRecord = sessionStore->loadSession(recipientId, deviceId);
    ByteArray     plaintext     = decrypt(sessionRecord, ciphertext);

    sessionStore->storeSession(recipientId, deviceId, sessionRecord);

    return plaintext;
}

ByteArray SessionCipher::decrypt(SessionRecord *sessionRecord, std::shared_ptr<WhisperMessage> ciphertext)
{
    std::vector<SessionState*> previousStatesList = sessionRecord->getPreviousSessionStates();
    std::vector<WhisperException> exceptions;

    try {
        SessionState *sessionState = sessionRecord->getSessionState();
        ByteArray    plaintext    = decrypt(sessionState, ciphertext);

        sessionRecord->setState(sessionState);
        return plaintext;
    } catch (const InvalidMessageException &e) {
        exceptions.push_back(e);
    }

	for (unsigned i = 0; i < previousStatesList.size(); i++) {
        try {
            SessionState *promotedState = previousStatesList[i];
            ByteArray    plaintext     = decrypt(promotedState, ciphertext);

            previousStatesList.erase(previousStatesList.begin() + i);
            sessionRecord->promoteState(promotedState);

            return plaintext;
        } catch (const InvalidMessageException &e) {
            exceptions.push_back(e);
        }
    }

    throw InvalidMessageException("No valid sessions.", exceptions);
}

ByteArray SessionCipher::decrypt(SessionState *sessionState, std::shared_ptr<WhisperMessage> ciphertextMessage)
{
    if (!sessionState->hasSenderChain()) {
        throw InvalidMessageException("Uninitialized session!");
    }

    if (ciphertextMessage->getMessageVersion() != sessionState->getSessionVersion()) {
        throw InvalidMessageException("Message version " + std::to_string(ciphertextMessage->getMessageVersion()) +
                                          ", but session version " +                                           
                                          std::to_string(sessionState->getSessionVersion()));
    }

    int            messageVersion    = ciphertextMessage->getMessageVersion();
    DjbECPublicKey theirEphemeral    = ciphertextMessage->getSenderRatchetKey();
    unsigned       counter           = ciphertextMessage->getCounter();
    ChainKey       chainKey          = getOrCreateChainKey(sessionState, theirEphemeral);
    MessageKeys    messageKeys       = getOrCreateMessageKeys(sessionState, theirEphemeral,
                                                              chainKey, counter);

    ciphertextMessage->verifyMac(messageVersion,
                                 sessionState->getRemoteIdentityKey(),
                                 sessionState->getLocalIdentityKey(),
                                 messageKeys.getMacKey());

    ByteArray plaintext = getPlaintext(messageVersion, messageKeys, ciphertextMessage->getBody());

    sessionState->clearUnacknowledgedPreKeyMessage();

    return plaintext;
}

int SessionCipher::getRemoteRegistrationId()
{
    SessionRecord *record = sessionStore->loadSession(recipientId, deviceId);
    return record->getSessionState()->getRemoteRegistrationId();
}

int SessionCipher::getSessionVersion()
{
    if (!sessionStore->containsSession(recipientId, deviceId)) {
        //qDebug() << "No session for" << recipientId << deviceId;
        throw NoSessionException("No session for (" + std::to_string(recipientId) + ", " + std::to_string(deviceId) + ")!");
    }

    SessionRecord *record = sessionStore->loadSession(recipientId, deviceId);
    return record->getSessionState()->getSessionVersion();
}

ChainKey SessionCipher::getOrCreateChainKey(SessionState *sessionState, const DjbECPublicKey &theirEphemeral)
{
    try {
        if (sessionState->hasReceiverChain(theirEphemeral)) {
            return sessionState->getReceiverChainKey(theirEphemeral);
        } else {
            RootKey                  rootKey         = sessionState->getRootKey();
            ECKeyPair                ourEphemeral    = sessionState->getSenderRatchetKeyPair();
            std::pair<RootKey, ChainKey> receiverChain   = rootKey.createChain(theirEphemeral, ourEphemeral);
            ECKeyPair                ourNewEphemeral = Curve::generateKeyPair();
            std::pair<RootKey, ChainKey> senderChain     = receiverChain.first.createChain(theirEphemeral, ourNewEphemeral);

            sessionState->setRootKey(senderChain.first);
            sessionState->addReceiverChain(theirEphemeral, receiverChain.second);
            sessionState->setPreviousCounter(std::max(sessionState->getSenderChainKey().getIndex() - 1, (unsigned)0));
            sessionState->setSenderChain(ourNewEphemeral, senderChain.second);

            return receiverChain.second;
        }
    } catch (const InvalidKeyException &e) {
        throw InvalidMessageException(__PRETTY_FUNCTION__, {e});
    }
}

MessageKeys SessionCipher::getOrCreateMessageKeys(SessionState *sessionState, const DjbECPublicKey &theirEphemeral, const ChainKey &chainKey, unsigned counter)
{
    if (chainKey.getIndex() > counter) {
        if (sessionState->hasMessageKeys(theirEphemeral, counter)) {
            return sessionState->removeMessageKeys(theirEphemeral, counter);
        } else {
            throw DuplicateMessageException("Received message with old counter: " +
                                                std::to_string(chainKey.getIndex()) + ", " +
                                                std::to_string(counter));
        }
    }

    if (counter - chainKey.getIndex() > 2000) {
        throw InvalidMessageException("Over 2000 messages into the future!");
    }

    ChainKey nowChainKey = chainKey;
    while (nowChainKey.getIndex() < counter) {
        MessageKeys messageKeys = nowChainKey.getMessageKeys();
        sessionState->setMessageKeys(theirEphemeral, messageKeys);
        nowChainKey = nowChainKey.getNextChainKey();
    }

    sessionState->setReceiverChainKey(theirEphemeral, nowChainKey.getNextChainKey());
    return nowChainKey.getMessageKeys();
}

ByteArray SessionCipher::getCiphertext(int version, const MessageKeys &messageKeys, const ByteArray &plaintext)
{
    AES_KEY enc_key;
    ByteArray key = messageKeys.getCipherKey();
    if (version >= 3) {
        AES_set_encrypt_key((const unsigned char*)key.c_str(), key.size() * 8, &enc_key);
        ByteArray padText = plaintext;
        int padlen = ((padText.size() + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE - plaintext.size();
        padText += ByteArray(padlen, (char)padlen);
        ByteArray out(padText.size(), '\0');
        ByteArray ivec(messageKeys.getIv());
        AES_cbc_encrypt((const unsigned char*)padText.c_str(), (unsigned char*)out.data(),
                        padText.size(), &enc_key,
                        (unsigned char*)ivec.data(), AES_ENCRYPT);
        return out;
    } else {
        /*AES_set_encrypt_key((const unsigned char*)key.c_str(), 128, &enc_key);
        ByteArray out(plaintext.size(), '\0');
        unsigned int counter = 0;
        ByteArray iv(AES_BLOCK_SIZE, '\0');
        //ByteUtil::intToByteArray(iv, 0, counter);
        unsigned char ecount[AES_BLOCK_SIZE];
        memset(ecount, 0, AES_BLOCK_SIZE);
        // TODO store state
        for (unsigned int i = 0; i < messageKeys.getCounter(); i++) {
            AES_encrypt((const unsigned char*)iv.c_str(), ecount, &enc_key);
            ctr128_inc_aligned((unsigned char*)iv.data());
        }
        AES_ctr128_encrypt((const unsigned char*)plaintext.c_str(), (unsigned char*)out.data(),
                           plaintext.size(), &enc_key, (unsigned char*)iv.data(),
                           ecount, &counter);
        return out;*/
		return "";
    }
}

ByteArray SessionCipher::getPlaintext(int version, const MessageKeys &messageKeys, const ByteArray &cipherText)
{
    AES_KEY dec_key;
    ByteArray key = messageKeys.getCipherKey();
    ByteArray out(cipherText.size(), '\0');
    if (version >= 3) {
        AES_set_decrypt_key((const unsigned char*)key.c_str(), key.size() * 8, &dec_key);
        ByteArray ivec(messageKeys.getIv());
        AES_cbc_encrypt((const unsigned char*)cipherText.c_str(),
                        (unsigned char*)out.data(),
                        cipherText.size(), &dec_key,
                        (unsigned char*)ivec.data(), AES_DECRYPT);
        out = out.substr(0, out.size() - (unsigned int)out[out.size() - 1]);
    } else {
        /*AES_set_encrypt_key((const unsigned char*)key.c_str(), 128, &dec_key);
        unsigned int counter = 0;
        ByteArray iv(AES_BLOCK_SIZE, '\0');
        //ByteUtil::intToByteArray(iv, 0, counter);
        unsigned char ecount[AES_BLOCK_SIZE];
        memset(ecount, 0, AES_BLOCK_SIZE);
        for (unsigned int i = 0; i < messageKeys.getCounter(); i++) {
            AES_encrypt((const unsigned char*)iv.c_str(), ecount, &dec_key);
            ctr128_inc_aligned((unsigned char*)iv.data());
        }
        AES_ctr128_encrypt((const unsigned char*)cipherText.c_str(), (unsigned char*)out.data(),
                           cipherText.size(), &dec_key, (unsigned char*)iv.data(),
                           ecount, &counter);*/
		out = "[ Message using AES CTR128, not implemented! ]";
    }
    return out;
}
