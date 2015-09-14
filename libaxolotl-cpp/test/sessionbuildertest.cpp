#include "sessionbuildertest.h"

#include "inmemoryaxolotlstore.h"

#include "sessionbuilder.h"
#include "ecc/curve.h"
#include "sessioncipher.h"

#include "untrustedidentityexception.h"
#include "invalidkeyidexception.h"
#include "invalidkeyexception.h"
#include "invalidmessageexception.h"

#include "state/LocalStorageProtocol.pb.h"
#include "protocol/WhisperTextProtocol.pb.h"

#include <iostream>
#include <memory>
#include <vector>

long SessionBuilderTest::ALICE_RECIPIENT_ID = 5;
long SessionBuilderTest::BOB_RECIPIENT_ID = 2;

SessionBuilderTest::SessionBuilderTest()
{
}

void SessionBuilderTest::testBasicPreKeyV2()
{
    try {
        std::cerr << "testBasicPreKeyV2" << std::endl;

        std::shared_ptr<AxolotlStore> aliceStore(new InMemoryAxolotlStore());
        SessionBuilder aliceSessionBuilder(aliceStore, SessionBuilderTest::BOB_RECIPIENT_ID, 1);

        std::shared_ptr<AxolotlStore> bobStore(new InMemoryAxolotlStore());
        ECKeyPair    bobPreKeyPair = Curve::generateKeyPair();
        PreKeyBundle bobPreKey(bobStore->getLocalRegistrationId(), 1,
                               31337, bobPreKeyPair.getPublicKey(),
                               0, DjbECPublicKey(), ByteArray(),
                               bobStore->getIdentityKeyPair().getPublicKey());

        aliceSessionBuilder.process(bobPreKey);

        bool containsSession = aliceStore->containsSession(BOB_RECIPIENT_ID, 1);
        int   sessionVersion = aliceStore->loadSession(BOB_RECIPIENT_ID, 1)->getSessionState()->getSessionVersion();

        bool passed1 = containsSession && sessionVersion == 2;

        std::cerr << "PASSED 1" << passed1 << std::endl;

        if (!passed1) {
            std::cerr << "don't know what to show..." << std::endl;
        }

        ByteArray        originalMessage("L'homme est condamné à être libre");
        SessionCipher    *aliceSessionCipher = new SessionCipher(aliceStore, BOB_RECIPIENT_ID, 1);
        std::shared_ptr<CiphertextMessage> outgoingMessage = aliceSessionCipher->encrypt(originalMessage);

        bool passed2 = outgoingMessage->getType() == CiphertextMessage::PREKEY_TYPE;

        std::cerr << "PASSED 2" << passed2 << std::endl;

        if (!passed2) {
            std::cerr << "CiphertextMessage::getType" << outgoingMessage->getType() << std::endl;
        }

        ByteArray outgoingMessageSerialized = outgoingMessage->serialize();

        std::shared_ptr<PreKeyWhisperMessage> incomingMessage(new PreKeyWhisperMessage(outgoingMessageSerialized));
        bobStore->storePreKey(31337, PreKeyRecord(bobPreKey.getPreKeyId(), bobPreKeyPair));


        SessionCipher bobSessionCipher(bobStore, ALICE_RECIPIENT_ID, 1);
        ByteArray plaintext = bobSessionCipher.decrypt(incomingMessage);

        bool passed3 = bobStore->containsSession(ALICE_RECIPIENT_ID, 1)
                && bobStore->loadSession(ALICE_RECIPIENT_ID, 1)->getSessionState()->getSessionVersion() == 2
                && originalMessage == plaintext;

        std::cerr << "PASSED 3" << passed3 << std::endl;

        if (!passed3) {
            std::cerr << "sessionVersion" << bobStore->loadSession(ALICE_RECIPIENT_ID, 1)->getSessionState()->getSessionVersion() << std::endl;
            std::cerr << "plaintext" << ByteUtil::toHex(plaintext) << std::endl;
        }
    }
    catch (const UntrustedIdentityException &e) {
        std::cerr << "UntrustedIdentityException" << e.errorMessage() << std::endl;
    }
    catch (const InvalidKeyIdException &e) {
        std::cerr << "InvalidKeyIdException" << e.errorMessage() << std::endl;
    }
    catch (const InvalidKeyException &e) {
        std::cerr << "InvalidKeyException" << e.errorMessage() << std::endl;
    }
    catch (const InvalidMessageException &e) {
        std::cerr << "InvalidMessageException" << e.errorMessage() << std::endl;
    }
}
