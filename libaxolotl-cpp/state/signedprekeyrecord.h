#ifndef SIGNEDPREKEYRECORD_H
#define SIGNEDPREKEYRECORD_H

#include "LocalStorageProtocol.pb.h"
#include "eckeypair.h"

class SignedPreKeyRecord
{
public:
    SignedPreKeyRecord(uint64_t id, uint64_t timestamp, const ECKeyPair &keyPair, const ByteArray &signature);
    SignedPreKeyRecord(const ByteArray &serialized);

    uint64_t getId() const;
    uint64_t getTimestamp() const;
    ECKeyPair getKeyPair() const;
    ByteArray getSignature() const;
    ByteArray serialize() const;

private:
    textsecure::SignedPreKeyRecordStructure structure;
};

#endif // SIGNEDPREKEYRECORD_H
