#ifndef SESSIONCIPHER_H
#define SESSIONCIPHER_H

#include <memory>

#include "state/sessionstore.h"
#include "sessionbuilder.h"
#include "ratchet/messagekeys.h"

class SessionCipher
{
public:
    SessionCipher(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore,
                  std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore,
                  uint64_t recipientId, int deviceId);
    SessionCipher(std::shared_ptr<AxolotlStore> store, uint64_t recipientId, int deviceId);
    std::shared_ptr<CiphertextMessage> encrypt(const ByteArray &paddedMessage);
    ByteArray decrypt(std::shared_ptr<PreKeyWhisperMessage> ciphertext);
    ByteArray decrypt(std::shared_ptr<WhisperMessage> ciphertext);
    ByteArray decrypt(SessionRecord *sessionRecord, std::shared_ptr<WhisperMessage> ciphertext);
    ByteArray decrypt(SessionState *sessionState, std::shared_ptr<WhisperMessage> ciphertextMessage);
    int getRemoteRegistrationId();
    int getSessionVersion() ;

private:
    void init(std::shared_ptr<SessionStore> sessionStore, std::shared_ptr<PreKeyStore> preKeyStore,
              std::shared_ptr<SignedPreKeyStore> signedPreKeyStore, std::shared_ptr<IdentityKeyStore> identityKeyStore,
              uint64_t recipientId, int deviceId);
    ChainKey getOrCreateChainKey(SessionState *sessionState, const DjbECPublicKey &theirEphemeral);
    MessageKeys getOrCreateMessageKeys(SessionState *sessionState,
                                       const DjbECPublicKey &theirEphemeral,
                                       const ChainKey &chainKey, uint counter);
    ByteArray getCiphertext(int version, const MessageKeys &messageKeys, const ByteArray &plaintext);
    ByteArray getPlaintext(int version, const MessageKeys &messageKeys, const ByteArray &cipherText);

    std::shared_ptr<SessionStore>   sessionStore;
    SessionBuilder sessionBuilder;
    std::shared_ptr<PreKeyStore>    preKeyStore;
    uint64_t           recipientId;
    int            deviceId;
};

#endif // SESSIONCIPHER_H
