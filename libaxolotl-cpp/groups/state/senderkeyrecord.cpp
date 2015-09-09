#include "senderkeyrecord.h"

#include "LocalStorageProtocol.pb.h"

#include "invalidkeyidexception.h"

SenderKeyRecord::SenderKeyRecord()
{

}

SenderKeyRecord::SenderKeyRecord(const ByteArray &serialized)
{

    textsecure::SenderKeyRecordStructure senderKeyRecordStructure;
    senderKeyRecordStructure.ParseFromArray(serialized.c_str(), serialized.size());

    for (int i = 0; i < senderKeyRecordStructure.senderkeystates_size(); i++) {
        textsecure::SenderKeyStateStructure structure = senderKeyRecordStructure.senderkeystates(i);
        SenderKeyState *state = new SenderKeyState(structure);
        senderKeyStates.push_back(state);
    }
}

SenderKeyState *SenderKeyRecord::getSenderKeyState(int keyId)
{
    if (!senderKeyStates.empty() && (keyId < senderKeyStates.size())) {
        return senderKeyStates.at(keyId);
    } else {
        throw InvalidKeyIdException("No key state " + std::to_string(keyId) + " in record!");
    }
}

void SenderKeyRecord::addSenderKeyState(int id, int iteration, const ByteArray &chainKey, const DjbECPublicKey &signatureKey)
{
    senderKeyStates.push_back(new SenderKeyState(id, iteration, chainKey, signatureKey));
}

void SenderKeyRecord::setSenderKeyState(int id, int iteration, const ByteArray &chainKey, const ECKeyPair &signatureKey)
{
    senderKeyStates.clear();
    senderKeyStates.push_back(new SenderKeyState(id, iteration, chainKey, signatureKey));
}

ByteArray SenderKeyRecord::serialize() const
{
    textsecure::SenderKeyRecordStructure recordStructure;

    for (SenderKeyState *senderKeyState: senderKeyStates) {
        recordStructure.add_senderkeystates()->CopyFrom(senderKeyState->getStructure());
    }

    ::std::string recordStructureString = recordStructure.SerializeAsString();
    ByteArray recordStructureBytes(recordStructureString.data(), recordStructureString.length());

    return recordStructureBytes;
}
