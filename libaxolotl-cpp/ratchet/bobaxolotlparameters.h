#ifndef BOBAXOLOTLPARAMETERS_H
#define BOBAXOLOTLPARAMETERS_H

#include "identitykeypair.h"
#include "eckeypair.h"

class BobAxolotlParameters
{
public:
    BobAxolotlParameters();
    BobAxolotlParameters(const IdentityKeyPair &ourIdentityKey, const ECKeyPair &ourSignedPreKey,
                         const ECKeyPair &ourOneTimePreKey, const ECKeyPair &ourRatchetKey,
                         const IdentityKey &theirIdentityKey, const DjbECPublicKey &theirBaseKey);

    IdentityKey getTheirIdentityKey() const;
    void setTheirIdentityKey(const IdentityKey &theirIdentityKey);
    DjbECPublicKey getTheirBaseKey() const;
    void setTheirBaseKey(const DjbECPublicKey &theirBaseKey);
    IdentityKeyPair getOurIdentityKey() const;
    void setOurIdentityKey(const IdentityKeyPair &ourIdentityKey);
    ECKeyPair getOurSignedPreKey() const;
    void setOurSignedPreKey(const ECKeyPair &ourSignedPreKey);
    ECKeyPair getOurOneTimePreKey() const;
    void setOurOneTimePreKey(const ECKeyPair &ourOneTimePreKey);
    ECKeyPair getOurRatchetKey() const;
    void setOurRatchetKey(const ECKeyPair &ourRatchetKey);

private:
    IdentityKeyPair ourIdentityKey;
    ECKeyPair ourSignedPreKey;
    ECKeyPair ourOneTimePreKey;
    ECKeyPair ourRatchetKey;

    IdentityKey theirIdentityKey;
    DjbECPublicKey theirBaseKey;

};

#endif // BOBAXOLOTLPARAMETERS_H
