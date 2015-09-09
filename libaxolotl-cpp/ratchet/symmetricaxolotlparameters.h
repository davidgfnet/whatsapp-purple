#ifndef SYMMETRICAXOLOTLPARAMETERS_H
#define SYMMETRICAXOLOTLPARAMETERS_H

#include "eckeypair.h"
#include "identitykeypair.h"

class SymmetricAxolotlParameters
{
public:
    SymmetricAxolotlParameters();
    SymmetricAxolotlParameters(const ECKeyPair &ourBaseKey, const ECKeyPair &ourRatchetKey,
                               const IdentityKeyPair &ourIdentityKey, const DjbECPublicKey &theirBaseKey,
                               const DjbECPublicKey &theirRatchetKey, const IdentityKey &theirIdentityKey);

    ECKeyPair getOurBaseKey() const;
    ECKeyPair getOurRatchetKey() const;
    IdentityKeyPair getOurIdentityKey() const;
    DjbECPublicKey getTheirBaseKey() const;
    DjbECPublicKey getTheirRatchetKey() const;
    IdentityKey getTheirIdentityKey() const;

    void setOurBaseKey(const ECKeyPair &ourBaseKey);
    void setOurRatchetKey(const ECKeyPair &ourRatchetKey);
    void setOurIdentityKey(const IdentityKeyPair &ourIdentityKey);
    void setTheirBaseKey(const DjbECPublicKey &theirBaseKey);
    void setTheirRatchetKey(const DjbECPublicKey &theirRatchetKey);
    void setTheirIdentityKey(const IdentityKey &theirIdentityKey);

private:
    ECKeyPair       ourBaseKey;
    ECKeyPair       ourRatchetKey;
    IdentityKeyPair ourIdentityKey;

    DjbECPublicKey     theirBaseKey;
    DjbECPublicKey     theirRatchetKey;
    IdentityKey     theirIdentityKey;

};

#endif // SYMMETRICAXOLOTLPARAMETERS_H
