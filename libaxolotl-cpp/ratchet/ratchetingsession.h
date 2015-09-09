#ifndef RATCHETINGSESSION_H
#define RATCHETINGSESSION_H

#include "byteutil.h"
#include "sessionstate.h"
#include "aliceaxolotlparameters.h"
#include "bobaxolotlparameters.h"
#include "symmetricaxolotlparameters.h"
#include "djbec.h"
#include "chainkey.h"
#include "rootkey.h"

class DerivedKeys
{
public:
    DerivedKeys() {}
    DerivedKeys(const RootKey &rootKey, const ChainKey &chainKey) {
        this->rootKey  = rootKey;
        this->chainKey = chainKey;
    }

    RootKey getRootKey() const {
        return rootKey;
    }

    ChainKey getChainKey() const {
        return chainKey;
    }

private:
    RootKey   rootKey;
    ChainKey  chainKey;
};

class RatchetingSession
{
public:
    RatchetingSession();

    static void initializeSession(SessionState *sessionState,
                                  int sessionVersion,
                                  const SymmetricAxolotlParameters &parameters);
    static void initializeSession(SessionState *sessionState,
                                  int sessionVersion,
                                  const AliceAxolotlParameters &parameters);
    static void initializeSession(SessionState *sessionState,
                                  int sessionVersion,
                                  const BobAxolotlParameters &parameters);

    static DerivedKeys calculateDerivedKeys(int sessionVersion, const ByteArray &masterSecret);
    static ByteArray getDiscontinuityBytes();
    static bool isAlice(const DjbECPublicKey &ourKey, const DjbECPublicKey &theirKey);
};

#endif // RATCHETINGSESSION_H
