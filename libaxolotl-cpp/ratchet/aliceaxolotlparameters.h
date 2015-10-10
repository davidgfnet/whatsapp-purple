#ifndef ALICEAXOLOTLPARAMETERS_H
#define ALICEAXOLOTLPARAMETERS_H

#include "identitykeypair.h"
#include "eckeypair.h"

class AliceAxolotlParameters
{
public:
    AliceAxolotlParameters();
    AliceAxolotlParameters(const IdentityKeyPair &ourIdentityKey, const ECKeyPair &ourBaseKey,
                           const IdentityKey &theirIdentityKey, const DjbECPublicKey &theirSignedPreKey,
                           const DjbECPublicKey &theirRatchetKey, const DjbECPublicKey &theirOneTimePreKey);

    IdentityKeyPair getOurIdentityKey() const;
    void setOurIdentityKey(const IdentityKeyPair &ourIdentityKey);
    ECKeyPair getOurBaseKey() const;
    void setOurBaseKey(const ECKeyPair &ourBaseKey);
    IdentityKey getTheirIdentityKey() const;
    void setTheirIdentityKey(const IdentityKey &theirIdentityKey);
    DjbECPublicKey getTheirSignedPreKey() const;
    void setTheirSignedPreKey(const DjbECPublicKey &theirSignedPreKey);
    DjbECPublicKey getTheirOneTimePreKey() const;
    void setTheirOneTimePreKey(const DjbECPublicKey &theirOneTimePreKey);
    DjbECPublicKey getTheirRatchetKey() const;
    void setTheirRatchetKey(const DjbECPublicKey &theirRatchetKey);

private:
    IdentityKeyPair ourIdentityKey;
    ECKeyPair ourBaseKey;

    IdentityKey theirIdentityKey;
    DjbECPublicKey theirSignedPreKey;
    DjbECPublicKey theirOneTimePreKey;
    DjbECPublicKey theirRatchetKey;

};

#endif // ALICEAXOLOTLPARAMETERS_H
