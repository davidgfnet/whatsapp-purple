#include "aliceaxolotlparameters.h"

AliceAxolotlParameters::AliceAxolotlParameters()
{

}

AliceAxolotlParameters::AliceAxolotlParameters(const IdentityKeyPair &ourIdentityKey, const ECKeyPair &ourBaseKey, const IdentityKey &theirIdentityKey, const DjbECPublicKey &theirSignedPreKey, const DjbECPublicKey &theirRatchetKey, const DjbECPublicKey &theirOneTimePreKey)
{
    this->ourIdentityKey     = ourIdentityKey;
    this->ourBaseKey         = ourBaseKey;
    this->theirIdentityKey   = theirIdentityKey;
    this->theirSignedPreKey  = theirSignedPreKey;
    this->theirRatchetKey    = theirRatchetKey;
    this->theirOneTimePreKey = theirOneTimePreKey;
}

IdentityKeyPair AliceAxolotlParameters::getOurIdentityKey() const
{
    return ourIdentityKey;
}

void AliceAxolotlParameters::setOurIdentityKey(const IdentityKeyPair &ourIdentityKey)
{
    this->ourIdentityKey = ourIdentityKey;
}

ECKeyPair AliceAxolotlParameters::getOurBaseKey() const
{
    return ourBaseKey;
}

void AliceAxolotlParameters::setOurBaseKey(const ECKeyPair &ourBaseKey)
{
    this->ourBaseKey = ourBaseKey;
}

IdentityKey AliceAxolotlParameters::getTheirIdentityKey() const
{
    return theirIdentityKey;
}

void AliceAxolotlParameters::setTheirIdentityKey(const IdentityKey &theirIdentityKey)
{
    this->theirIdentityKey = theirIdentityKey;
}

DjbECPublicKey AliceAxolotlParameters::getTheirSignedPreKey() const
{
    return theirSignedPreKey;
}

void AliceAxolotlParameters::setTheirSignedPreKey(const DjbECPublicKey &theirSignedPreKey)
{
    this->theirSignedPreKey = theirSignedPreKey;
}

DjbECPublicKey AliceAxolotlParameters::getTheirOneTimePreKey() const
{
    return theirOneTimePreKey;
}

void AliceAxolotlParameters::setTheirOneTimePreKey(const DjbECPublicKey &theirOneTimePreKey)
{
    this->theirOneTimePreKey = theirOneTimePreKey;
}

DjbECPublicKey AliceAxolotlParameters::getTheirRatchetKey() const
{
    return theirRatchetKey;
}

void AliceAxolotlParameters::setTheirRatchetKey(const DjbECPublicKey &theirRatchetKey)
{
    this->theirRatchetKey = theirRatchetKey;
}
