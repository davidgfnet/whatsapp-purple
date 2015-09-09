#ifndef PREKEYSTORE_H
#define PREKEYSTORE_H

#include "prekeyrecord.h"

class PreKeyStore {
public:
    virtual PreKeyRecord loadPreKey(uint64_t preKeyId) = 0;
    virtual void         storePreKey(uint64_t preKeyId, const PreKeyRecord &record) = 0;
    virtual bool         containsPreKey(uint64_t preKeyId) = 0;
    virtual void         removePreKey(uint64_t preKeyId) = 0;
    virtual int          countPreKeys() = 0;
};

#endif // PREKEYSTORE_H
