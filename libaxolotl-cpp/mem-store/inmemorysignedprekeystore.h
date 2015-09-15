#ifndef INMEMORYSIGNEDPREKEYSTORE_H
#define INMEMORYSIGNEDPREKEYSTORE_H

#include "state/signedprekeystore.h"

#include <map>
#include <vector>
#include "byteutil.h"

class InMemorySignedPreKeyStore : public SignedPreKeyStore
{
public:
    InMemorySignedPreKeyStore();
    SignedPreKeyRecord loadSignedPreKey(uint64_t signedPreKeyId);
    std::vector<SignedPreKeyRecord> loadSignedPreKeys();
    void storeSignedPreKey(uint64_t signedPreKeyId, const SignedPreKeyRecord &record);
    bool containsSignedPreKey(uint64_t signedPreKeyId);
    void removeSignedPreKey(uint64_t signedPreKeyId);

	std::string serialize() const;

private:
    std::map<uint64_t, ByteArray> store;
};

#endif // INMEMORYSIGNEDPREKEYSTORE_H

