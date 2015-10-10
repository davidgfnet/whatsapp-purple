#include "bobaxolotlparameters.h"

BobAxolotlParameters::BobAxolotlParameters()
{

}

BobAxolotlParameters::BobAxolotlParameters(const IdentityKeyPair &ourIdentityKey, const ECKeyPair &ourSignedPreKey, const ECKeyPair &ourOneTimePreKey, const ECKeyPair &ourRatchetKey, const IdentityKey &theirIdentityKey, const DjbECPublicKey &theirBaseKey)
{
    this->ourIdentityKey   = ourIdentityKey;
    this->ourSignedPreKey  = ourSignedPreKey;
    this->ourRatchetKey    = ourRatchetKey;
    this->ourOneTimePreKey = ourOneTimePreKey;
    this->theirIdentityKey = theirIdentityKey;
    this->theirBaseKey     = theirBaseKey;
}

IdentityKey BobAxolotlParameters::getTheirIdentityKey() const
{
    return theirIdentityKey;
}

void BobAxolotlParameters::setTheirIdentityKey(const IdentityKey &theirIdentityKey)
{
    this->theirIdentityKey = theirIdentityKey;
}

DjbECPublicKey BobAxolotlParameters::getTheirBaseKey() const
{
    return theirBaseKey;
}

void BobAxolotlParameters::setTheirBaseKey(const DjbECPublicKey &theirBaseKey)
{
    this->theirBaseKey = theirBaseKey;
}

IdentityKeyPair BobAxolotlParameters::getOurIdentityKey() const
{
    return ourIdentityKey;
}

void BobAxolotlParameters::setOurIdentityKey(const IdentityKeyPair &ourIdentityKey)
{
    this->ourIdentityKey = ourIdentityKey;
}

ECKeyPair BobAxolotlParameters::getOurSignedPreKey() const
{
    return ourSignedPreKey;
}

void BobAxolotlParameters::setOurSignedPreKey(const ECKeyPair &ourSignedPreKey)
{
    this->ourSignedPreKey = ourSignedPreKey;
}

ECKeyPair BobAxolotlParameters::getOurOneTimePreKey() const
{
    return ourOneTimePreKey;
}

void BobAxolotlParameters::setOurOneTimePreKey(const ECKeyPair &ourOneTimePreKey)
{
    this->ourOneTimePreKey = ourOneTimePreKey;
}

ECKeyPair BobAxolotlParameters::getOurRatchetKey() const
{
    return ourRatchetKey;
}

void BobAxolotlParameters::setOurRatchetKey(const ECKeyPair &ourRatchetKey)
{
    this->ourRatchetKey = ourRatchetKey;
}
