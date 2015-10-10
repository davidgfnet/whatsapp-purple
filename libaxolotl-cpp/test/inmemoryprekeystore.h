#ifndef INMEMORYPREKEYSTORE_H
#define INMEMORYPREKEYSTORE_H

#include "state/prekeystore.h"

#include <map>
#include "byteutil.h"

class InMemoryPreKeyStore : public PreKeyStore
{
public:
    InMemoryPreKeyStore();
    PreKeyRecord loadPreKey(uint64_t preKeyId);
    void         storePreKey(uint64_t preKeyId, const PreKeyRecord &record);
    bool         containsPreKey(uint64_t preKeyId);
    void         removePreKey(uint64_t preKeyId);
    int          countPreKeys() { return store.size(); }

private:
    std::map<uint64_t, ByteArray> store;
};

#endif // INMEMORYPREKEYSTORE_H
