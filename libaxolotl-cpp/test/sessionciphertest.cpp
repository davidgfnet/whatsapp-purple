#include "sessionciphertest.h"

#include "ecc/curve.h"
#include "ecc/djbec.h"
#include "ecc/eckeypair.h"
#include "ratchet/aliceaxolotlparameters.h"
#include "ratchet/bobaxolotlparameters.h"
#include "ratchet/ratchetingsession.h"
#include "sessioncipher.h"
#include "protocol/whispermessage.h"

#include "inmemoryaxolotlstore.h"

#include <memory>
#include <iostream>

#include <openssl/aes.h>

SessionCipherTest::SessionCipherTest()
{
}

void SessionCipherTest::testBasicSessionV2()
{
    std::cerr << "testBasicSessionV2" << std::endl;

    SessionRecord *aliceSessionRecord = new SessionRecord();
    SessionRecord *bobSessionRecord   = new SessionRecord();

    initializeSessionsV2(aliceSessionRecord->getSessionState(), bobSessionRecord->getSessionState());
    runInteraction(aliceSessionRecord, bobSessionRecord);
}

void SessionCipherTest::testBasicSessionV3()
{
    std::cerr << "testBasicSessionV3" << std::endl;

    SessionRecord *aliceSessionRecord = new SessionRecord();
    SessionRecord *bobSessionRecord   = new SessionRecord();

    initializeSessionsV3(aliceSessionRecord->getSessionState(), bobSessionRecord->getSessionState());
    runInteraction(aliceSessionRecord, bobSessionRecord);
}

void SessionCipherTest::runInteraction(SessionRecord *aliceSessionRecord, SessionRecord *bobSessionRecord)
{
    std::shared_ptr<AxolotlStore> aliceStore(new InMemoryAxolotlStore());
    std::shared_ptr<AxolotlStore> bobStore(new InMemoryAxolotlStore());

    aliceStore->storeSession(2, 1, aliceSessionRecord);
    bobStore->storeSession(3, 1, bobSessionRecord);

    SessionCipher     aliceCipher(aliceStore, 2, 1);
    SessionCipher     bobCipher(bobStore, 3, 1);

    bool test0 = true;
    for (int i = 0; i < 30; i++) {
        ByteArray        alicePlaintext("This is a plaintext message.");
        std::shared_ptr<CiphertextMessage> message = aliceCipher.encrypt(alicePlaintext);
        std::shared_ptr<WhisperMessage> whisperBob(new WhisperMessage(message->serialize()));
        ByteArray        bobPlaintext   = bobCipher.decrypt(whisperBob);
        if (bobPlaintext != alicePlaintext) {
            std::cerr << "FAILED AT " << i << ByteUtil::toHex(whisperBob->getBody()) << std::endl;
            std::cerr << "SOURCETEXT: " << ByteUtil::toHex(alicePlaintext) << std::endl;
            std::cerr << "PLAINTEXT: " << ByteUtil::toHex(bobPlaintext) << std::endl;
            test0 = false;
        }
    }

    std::cerr << "TEST0 " << (test0 ? "PASSED" : "FAILED") << std::endl;

    ByteArray        alicePlaintext("This is a plaintext message.");
    std::shared_ptr<CiphertextMessage> message = aliceCipher.encrypt(alicePlaintext);
    std::shared_ptr<WhisperMessage> whisperBob(new WhisperMessage(message->serialize()));
    ByteArray        bobPlaintext   = bobCipher.decrypt(whisperBob);

    bool passed1 = alicePlaintext == bobPlaintext;
    std::cerr << "PASSED 1 " << passed1 << std::endl;

    if (!passed1) {
        std::cerr << "bobPlaintext" << ByteUtil::toHex(bobPlaintext) << std::endl;
    }

    ByteArray        bobReply("This is a message from Bob.");
    std::shared_ptr<CiphertextMessage> reply = bobCipher.encrypt(bobReply);
    std::shared_ptr<WhisperMessage> whisperAlice(new WhisperMessage(reply->serialize()));
    ByteArray        receivedReply = aliceCipher.decrypt(whisperAlice);

    bool passed2 = bobReply == receivedReply;
    std::cerr << "PASSED 2 " << passed2 << std::endl;

    if (!passed2) {
        std::cerr << "receivedReply" << ByteUtil::toHex(receivedReply) << std::endl;
    }

    std::vector< std::shared_ptr<CiphertextMessage> > aliceCiphertextMessages;
    std::vector<ByteArray> alicePlaintextMessages;

    for (int i = 0; i < 50; i++) {
        ByteArray msg = "test message no " + std::to_string(i);
        alicePlaintextMessages.push_back(msg);
        aliceCiphertextMessages.push_back(aliceCipher.encrypt(msg));
    }

    //long seed = QDateTime::currentMSecsSinceEpoch();

    // X Collections.shuffle(aliceCiphertextMessages, new Random(seed));
    // X Collections.shuffle(alicePlaintextMessages, new Random(seed));

    bool test3 = true;
    for (int i = 0; i < aliceCiphertextMessages.size() / 2; i++) {
        std::shared_ptr<WhisperMessage> decryptMessage(new WhisperMessage(aliceCiphertextMessages[i]->serialize()));
        ByteArray receivedPlaintext = bobCipher.decrypt(decryptMessage);

        bool passed3 = receivedPlaintext == alicePlaintextMessages[i];

        if (!passed3) {
            std::cerr << "receivedPlaintext[" + std::to_string(i) + "]     " << ByteUtil::toHex(receivedPlaintext) << std::endl;
            std::cerr << "alicePlaintextMessages[" + std::to_string(i) + "]" << ByteUtil::toHex(alicePlaintextMessages[i]) << std::endl;
            test3 = false;
        }
    }
    std::cerr << "TEST3 " << (test3 ? "PASSED" : "FAILED") << std::endl;

    std::vector< std::shared_ptr<CiphertextMessage> > bobCiphertextMessages;
    std::vector<ByteArray> bobPlaintextMessages;

    for (int i = 0; i < 20; i++) {
        ByteArray msg = "test message no " + std::to_string(i);
        bobPlaintextMessages.push_back(msg);
        bobCiphertextMessages.push_back(bobCipher.encrypt(msg));
    }

    //seed = QDateTime::currentMSecsSinceEpoch();

    // X Collections.shuffle(bobCiphertextMessages, new Random(seed));
    // X Collections.shuffle(bobPlaintextMessages, new Random(seed));

    bool test4 = true;
    for (int i = 0; i < bobCiphertextMessages.size() / 2; i++) {
        std::shared_ptr<WhisperMessage> decryptMessage(new WhisperMessage(bobCiphertextMessages[i]->serialize()));
        ByteArray receivedPlaintext = aliceCipher.decrypt(decryptMessage);

        bool passed4 = receivedPlaintext == bobPlaintextMessages[i];
        //std::cerr << "PASSED 4" << passed4;

        if (!passed4) {
            std::cerr << "receivedPlaintext[" + std::to_string(i) << "]   " << ByteUtil::toHex(receivedPlaintext) << std::endl;
            std::cerr << "bobPlaintextMessages[" + std::to_string(i) << "]" << ByteUtil::toHex(bobPlaintextMessages[i]) << std::endl;
            test4 = false;
        }
    }
    std::cerr << "TEST4 " << (test4 ? "PASSED" : "FAILED") << std::endl;

    bool test5 = true;
    for (int i = aliceCiphertextMessages.size() / 2; i < aliceCiphertextMessages.size(); i++) {
        std::shared_ptr<WhisperMessage> decryptMessage(new WhisperMessage(aliceCiphertextMessages[i]->serialize()));
        ByteArray receivedPlaintext = bobCipher.decrypt(decryptMessage);

        bool passed5 = receivedPlaintext == alicePlaintextMessages[i];
        //std::cerr << "PASSED 5" << passed5;

        if (!passed5) {
            std::cerr << "receivedPlaintext[" + std::to_string(i) + "]     " << ByteUtil::toHex(receivedPlaintext) << std::endl;
            std::cerr << "alicePlaintextMessages[" + std::to_string(i) + "]" << ByteUtil::toHex(alicePlaintextMessages[i]) << std::endl;
            test5 = true;
        }
    }
    std::cerr << "TEST5 " << (test5 ? "PASSED" : "FAILED") << std::endl;

    bool test6 = true;
    for (int i = bobCiphertextMessages.size() / 2; i < bobCiphertextMessages.size(); i++) {
        std::shared_ptr<WhisperMessage> decryptMessage(new WhisperMessage(bobCiphertextMessages[i]->serialize()));
        ByteArray receivedPlaintext = aliceCipher.decrypt(decryptMessage);

        bool passed6 = receivedPlaintext == bobPlaintextMessages[i];
        //std::cerr << "PASSED 6" << passed6;

        if (!passed6) {
            std::cerr << "receivedPlaintext[" + std::to_string(i) + "]   " << ByteUtil::toHex(receivedPlaintext) << std::endl;
            std::cerr << "bobPlaintextMessages[" + std::to_string(i) + "]" << ByteUtil::toHex(bobPlaintextMessages[i]) << std::endl;
            test6 = false;
        }
    }
    std::cerr << "TEST6 " << (test6 ? "PASSED" : "FAILED") << std::endl;
}

void SessionCipherTest::initializeSessionsV2(SessionState *aliceSessionState, SessionState *bobSessionState)
{
    ECKeyPair       aliceIdentityKeyPair = Curve::generateKeyPair();
    IdentityKeyPair aliceIdentityKey(IdentityKey(aliceIdentityKeyPair.getPublicKey()),
                                     aliceIdentityKeyPair.getPrivateKey());
    ECKeyPair       aliceBaseKey         = Curve::generateKeyPair();
    ECKeyPair       aliceEphemeralKey    = Curve::generateKeyPair();

    ECKeyPair       bobIdentityKeyPair   = Curve::generateKeyPair();
    IdentityKeyPair bobIdentityKey(IdentityKey(bobIdentityKeyPair.getPublicKey()),
                                   bobIdentityKeyPair.getPrivateKey());
    ECKeyPair       bobBaseKey           = Curve::generateKeyPair();
    ECKeyPair       bobEphemeralKey      = bobBaseKey;

    AliceAxolotlParameters aliceParameters;
    aliceParameters.setOurIdentityKey(aliceIdentityKey);
    aliceParameters.setOurBaseKey(aliceBaseKey);
    aliceParameters.setTheirIdentityKey(bobIdentityKey.getPublicKey());
    aliceParameters.setTheirSignedPreKey(bobEphemeralKey.getPublicKey());
    aliceParameters.setTheirRatchetKey(bobEphemeralKey.getPublicKey());

    BobAxolotlParameters bobParameters;
    bobParameters.setOurIdentityKey(bobIdentityKey);
    bobParameters.setOurRatchetKey(bobEphemeralKey);
    bobParameters.setOurSignedPreKey(bobBaseKey);
    bobParameters.setTheirBaseKey(aliceBaseKey.getPublicKey());
    bobParameters.setTheirIdentityKey(aliceIdentityKey.getPublicKey());

    RatchetingSession::initializeSession(aliceSessionState, 2, aliceParameters);
    RatchetingSession::initializeSession(bobSessionState, 2, bobParameters);
}

void SessionCipherTest::initializeSessionsV3(SessionState *aliceSessionState, SessionState *bobSessionState)
{
    ECKeyPair       aliceIdentityKeyPair = Curve::generateKeyPair();
    IdentityKeyPair aliceIdentityKey(IdentityKey(aliceIdentityKeyPair.getPublicKey()),
                                     aliceIdentityKeyPair.getPrivateKey());
    ECKeyPair       aliceBaseKey         = Curve::generateKeyPair();
    ECKeyPair       aliceEphemeralKey    = Curve::generateKeyPair();

    ECKeyPair       alicePreKey          = aliceBaseKey;

    ECKeyPair       bobIdentityKeyPair   = Curve::generateKeyPair();
    IdentityKeyPair bobIdentityKey(IdentityKey(bobIdentityKeyPair.getPublicKey()),
                                   bobIdentityKeyPair.getPrivateKey());
    ECKeyPair       bobBaseKey           = Curve::generateKeyPair();
    ECKeyPair       bobEphemeralKey      = bobBaseKey;

    ECKeyPair       bobPreKey            = Curve::generateKeyPair();

    AliceAxolotlParameters aliceParameters;
    aliceParameters.setOurBaseKey(aliceBaseKey);
    aliceParameters.setOurIdentityKey(aliceIdentityKey);
    aliceParameters.setTheirRatchetKey(bobEphemeralKey.getPublicKey());
    aliceParameters.setTheirSignedPreKey(bobBaseKey.getPublicKey());
    aliceParameters.setTheirIdentityKey(bobIdentityKey.getPublicKey());

    BobAxolotlParameters bobParameters;
    bobParameters.setOurRatchetKey(bobEphemeralKey);
    bobParameters.setOurSignedPreKey(bobBaseKey);
    bobParameters.setOurIdentityKey(bobIdentityKey);
    bobParameters.setTheirIdentityKey(aliceIdentityKey.getPublicKey());
    bobParameters.setTheirBaseKey(aliceBaseKey.getPublicKey());

    RatchetingSession::initializeSession(aliceSessionState, 3, aliceParameters);
    RatchetingSession::initializeSession(bobSessionState, 3, bobParameters);
}
