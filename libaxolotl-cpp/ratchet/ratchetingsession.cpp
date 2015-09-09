#include "ratchetingsession.h"
#include "byteutil.h"
#include "curve.h"
#include "djbec.h"

RatchetingSession::RatchetingSession()
{
}

void RatchetingSession::initializeSession(SessionState *sessionState, int sessionVersion, const SymmetricAxolotlParameters &parameters)
{
    if (RatchetingSession::isAlice(parameters.getOurBaseKey().getPublicKey(), parameters.getTheirBaseKey())) {
        AliceAxolotlParameters aliceParameters;
        aliceParameters.setOurBaseKey(parameters.getOurBaseKey());
        aliceParameters.setOurIdentityKey(parameters.getOurIdentityKey());
        aliceParameters.setTheirRatchetKey(parameters.getTheirRatchetKey());
        aliceParameters.setTheirIdentityKey(parameters.getTheirIdentityKey());
        aliceParameters.setTheirSignedPreKey(parameters.getTheirBaseKey());

        RatchetingSession::initializeSession(sessionState, sessionVersion, aliceParameters);
    }
}

void RatchetingSession::initializeSession(SessionState *sessionState, int sessionVersion, const AliceAxolotlParameters &parameters)
{
    sessionState->setSessionVersion(sessionVersion);
    sessionState->setRemoteIdentityKey(parameters.getTheirIdentityKey());
    sessionState->setLocalIdentityKey(parameters.getOurIdentityKey().getPublicKey());

    ECKeyPair sendingRatchetKey = Curve::generateKeyPair();
    ByteArray secrets;

    if (sessionVersion >= 3) {
        secrets += RatchetingSession::getDiscontinuityBytes();
    }

    secrets += Curve::calculateAgreement(parameters.getTheirSignedPreKey(),
                                             parameters.getOurIdentityKey().getPrivateKey());
    secrets += Curve::calculateAgreement(parameters.getTheirIdentityKey().getPublicKey(),
                                             parameters.getOurBaseKey().getPrivateKey());
    secrets += Curve::calculateAgreement(parameters.getTheirSignedPreKey(),
                                             parameters.getOurBaseKey().getPrivateKey());

    if (sessionVersion >= 3 && !parameters.getTheirOneTimePreKey().serialize().empty()) {
        secrets += Curve::calculateAgreement(parameters.getTheirOneTimePreKey(),
                                                 parameters.getOurBaseKey().getPrivateKey());
    }

    DerivedKeys              derivedKeys  = RatchetingSession::calculateDerivedKeys(sessionVersion, secrets);
    std::pair<RootKey, ChainKey> sendingChain = derivedKeys.getRootKey().createChain(parameters.getTheirRatchetKey(), sendingRatchetKey);

    sessionState->addReceiverChain(parameters.getTheirRatchetKey(), derivedKeys.getChainKey());
    sessionState->setSenderChain(sendingRatchetKey, sendingChain.second);
    sessionState->setRootKey(sendingChain.first);
}

void RatchetingSession::initializeSession(SessionState *sessionState, int sessionVersion, const BobAxolotlParameters &parameters)
{
    sessionState->setSessionVersion(sessionVersion);
    sessionState->setRemoteIdentityKey(parameters.getTheirIdentityKey());
    sessionState->setLocalIdentityKey(parameters.getOurIdentityKey().getPublicKey());

    ByteArray secrets;

    if (sessionVersion >= 3) {
        secrets += RatchetingSession::getDiscontinuityBytes();
    }

    secrets += Curve::calculateAgreement(parameters.getTheirIdentityKey().getPublicKey(),
                                             parameters.getOurSignedPreKey().getPrivateKey());
    secrets += Curve::calculateAgreement(parameters.getTheirBaseKey(),
                                             parameters.getOurIdentityKey().getPrivateKey());
    secrets += Curve::calculateAgreement(parameters.getTheirBaseKey(),
                                             parameters.getOurSignedPreKey().getPrivateKey());

    if (sessionVersion >= 3
            && !parameters.getOurOneTimePreKey().getPrivateKey().serialize().empty()
            && !parameters.getOurOneTimePreKey().getPublicKey().serialize().empty()) {
        secrets += Curve::calculateAgreement(parameters.getTheirBaseKey(),
                                                 parameters.getOurOneTimePreKey().getPrivateKey());
    }

    DerivedKeys              derivedKeys  = RatchetingSession::calculateDerivedKeys(sessionVersion, secrets);

    sessionState->setSenderChain(parameters.getOurRatchetKey(), derivedKeys.getChainKey());
    sessionState->setRootKey(derivedKeys.getRootKey());
}

DerivedKeys RatchetingSession::calculateDerivedKeys(int sessionVersion, const ByteArray &masterSecret)
{
    HKDF kdf(sessionVersion);
    ByteArray derivedSecretBytes = kdf.deriveSecrets(masterSecret, ByteArray("WhisperText"), 64);
    ByteArray rootSecrets = derivedSecretBytes.substr(0, 32);
    ByteArray chainSecrets = derivedSecretBytes.substr(32, 32);
    return DerivedKeys(RootKey(kdf, rootSecrets),
                       ChainKey(kdf, chainSecrets, 0));
}

ByteArray RatchetingSession::getDiscontinuityBytes()
{
    return ByteArray(32, '\xFF');
}

bool RatchetingSession::isAlice(const DjbECPublicKey &ourKey, const DjbECPublicKey &theirKey)
{
    return ourKey.serialize() < theirKey.serialize();
}
