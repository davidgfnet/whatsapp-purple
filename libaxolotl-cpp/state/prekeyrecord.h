#ifndef PREKEYRECORD_H
#define PREKEYRECORD_H

#include "byteutil.h"

#include "LocalStorageProtocol.pb.h"
#include "curve.h"
#include "eckeypair.h"

class PreKeyRecord
{
public:
    PreKeyRecord(uint64_t id, const ECKeyPair &keyPair);
    PreKeyRecord(const ByteArray &serialized);

    uint64_t getId() const;
    ECKeyPair getKeyPair() const;
    ByteArray serialize() const;

private:
    textsecure::PreKeyRecordStructure structure;

};

#endif // PREKEYRECORD_H
