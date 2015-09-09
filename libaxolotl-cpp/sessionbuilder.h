#ifndef SESSIONBUILDER_H
#define SESSIONBUILDER_H

#include <memory>

#include "state/sessionstore.h"
#include "state/signedprekeystore.h"
#include "state/prekeybundle.h"
#include "state/prekeystore.h"
#include "state/identitykeystore.h"
#include "state/axolotlstore.h"
#include "protocol/prekeywhispermessage.h"
#include "protocol/keyexchangemessage.h"

class SessionBuilder
{
public:
    SessionBuilder();
    SessionBuilder(std::shared_ptr<SessionStore> sessionStore,
                   std::shared_ptr<PreKeyStore> preKeyStore,
                   std::shared_ptr<SignedPreKeyStore> signedPreKeyStore,
                   std::shared_ptr<IdentityKeyStore> identityKeyStore,
                   uint64_t recipientId, int deviceId);
    SessionBuilder(std::shared_ptr<AxolotlStore> store, uint64_t recipientId, int deviceId);

    uint64_t process(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message);
    uint64_t processV3(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message);
    uint64_t processV2(SessionRecord *sessionRecord, std::shared_ptr<PreKeyWhisperMessage> message);
    void process(const PreKeyBundle &preKey);
    KeyExchangeMessage process(std::shared_ptr<KeyExchangeMessage> message);
    KeyExchangeMessage process();

private:
    void init(std::shared_ptr<SessionStore> sessionStore,
              std::shared_ptr<PreKeyStore> preKeyStore,
              std::shared_ptr<SignedPreKeyStore> signedPreKeyStore,
              std::shared_ptr<IdentityKeyStore> identityKeyStore,
              uint64_t recipientId, int deviceId);
    KeyExchangeMessage processInitiate(std::shared_ptr<KeyExchangeMessage> message);
    void processResponse(std::shared_ptr<KeyExchangeMessage> message);

private:
    std::shared_ptr<SessionStore>      sessionStore;
    std::shared_ptr<PreKeyStore>       preKeyStore;
    std::shared_ptr<SignedPreKeyStore> signedPreKeyStore;
    std::shared_ptr<IdentityKeyStore>  identityKeyStore;
    uint64_t   recipientId;
    int    deviceId;
};

#endif // SESSIONBUILDER_H
