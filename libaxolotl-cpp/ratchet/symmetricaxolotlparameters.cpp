#include "symmetricaxolotlparameters.h"

SymmetricAxolotlParameters::SymmetricAxolotlParameters()
{

}

SymmetricAxolotlParameters::SymmetricAxolotlParameters(const ECKeyPair &ourBaseKey, const ECKeyPair &ourRatchetKey, const IdentityKeyPair &ourIdentityKey, const DjbECPublicKey &theirBaseKey, const DjbECPublicKey &theirRatchetKey, const IdentityKey &theirIdentityKey)
{
    this->ourBaseKey       = ourBaseKey;
    this->ourRatchetKey    = ourRatchetKey;
    this->ourIdentityKey   = ourIdentityKey;
    this->theirBaseKey     = theirBaseKey;
    this->theirRatchetKey  = theirRatchetKey;
    this->theirIdentityKey = theirIdentityKey;
}

ECKeyPair SymmetricAxolotlParameters::getOurBaseKey() const
{
    return ourBaseKey;
}

ECKeyPair SymmetricAxolotlParameters::getOurRatchetKey() const
{
    return ourRatchetKey;
}

IdentityKeyPair SymmetricAxolotlParameters::getOurIdentityKey() const
{
    return ourIdentityKey;
}

DjbECPublicKey SymmetricAxolotlParameters::getTheirBaseKey() const
{
    return theirBaseKey;
}

DjbECPublicKey SymmetricAxolotlParameters::getTheirRatchetKey() const
{
    return theirRatchetKey;
}

IdentityKey SymmetricAxolotlParameters::getTheirIdentityKey() const
{
    return theirIdentityKey;
}

void SymmetricAxolotlParameters::setOurBaseKey(const ECKeyPair &ourBaseKey)
{
    this->ourBaseKey = ourBaseKey;
}

void SymmetricAxolotlParameters::setOurRatchetKey(const ECKeyPair &ourRatchetKey)
{
    this->ourRatchetKey = ourRatchetKey;
}

void SymmetricAxolotlParameters::setOurIdentityKey(const IdentityKeyPair &ourIdentityKey)
{
    this->ourIdentityKey = ourIdentityKey;
}

void SymmetricAxolotlParameters::setTheirBaseKey(const DjbECPublicKey &theirBaseKey)
{
    this->theirBaseKey = theirBaseKey;
}

void SymmetricAxolotlParameters::setTheirRatchetKey(const DjbECPublicKey &theirRatchetKey)
{
    this->theirRatchetKey = theirRatchetKey;
}

void SymmetricAxolotlParameters::setTheirIdentityKey(const IdentityKey &theirIdentityKey)
{
    this->theirIdentityKey = theirIdentityKey;
}
