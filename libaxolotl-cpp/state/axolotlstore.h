#ifndef AXOLOTLSTORE_H
#define AXOLOTLSTORE_H

#include "identitykeystore.h"
#include "prekeystore.h"
#include "sessionstore.h"
#include "signedprekeystore.h"

class AxolotlStore: public IdentityKeyStore, public PreKeyStore, public SessionStore, public SignedPreKeyStore {

};

#endif // AXOLOTLSTORE_H
