#include "senderkeystate.h"
#include "curve.h"
#include "byteutil.h"

SenderKeyState::SenderKeyState()
{

}

SenderKeyState::SenderKeyState(int id, int iteration, const ByteArray &chainKey, const DjbECPublicKey &signatureKey)
{
    senderKeyStateStructure = textsecure::SenderKeyStateStructure();
    senderKeyStateStructure.set_senderkeyid(id);
    senderKeyStateStructure.mutable_senderchainkey()->set_iteration(iteration);
    senderKeyStateStructure.mutable_senderchainkey()->set_seed(chainKey.c_str(),
                                                               chainKey.size());
    senderKeyStateStructure.mutable_sendersigningkey()->set_public_(signatureKey.serialize().c_str(),
                                                                    signatureKey.serialize().size());

    /*textsecure::SenderKeyStateStructure::SenderChainKey senderChainKeyStructure;
    senderChainKeyStructure.set_iteration(iteration);
    senderChainKeyStructure.set_seed(chainKey.c_str());

    textsecure::SenderKeyStateStructure::SenderSigningKey signingKeyStructure;
    signingKeyStructure.set_public_(signatureKey.serialize().c_str());

    senderKeyStateStructure = textsecure::SenderKeyStateStructure();
    senderKeyStateStructure.set_senderkeyid(id);
    senderKeyStateStructure.mutable_senderchainkey()->CopyFrom(senderChainKeyStructure);
    senderKeyStateStructure.mutable_sendersigningkey()->CopyFrom(signingKeyStructure);*/
}

SenderKeyState::SenderKeyState(int id, int iteration, const ByteArray &chainKey, const ECKeyPair &signatureKey)
{
    SenderKeyState(id, iteration, chainKey, signatureKey.getPublicKey(), signatureKey.getPrivateKey());
}

SenderKeyState::SenderKeyState(int id, int iteration, const ByteArray &chainKey, const DjbECPublicKey &signatureKeyPublic, const DjbECPrivateKey &signatureKeyPrivate)
{
    senderKeyStateStructure = textsecure::SenderKeyStateStructure();
    senderKeyStateStructure.set_senderkeyid(id);
    senderKeyStateStructure.mutable_senderchainkey()->set_iteration(iteration);
    senderKeyStateStructure.mutable_senderchainkey()->set_seed(chainKey.c_str(),
                                                               chainKey.size());
    senderKeyStateStructure.mutable_sendersigningkey()->set_public_(signatureKeyPublic.serialize().c_str(),
                                                                    signatureKeyPublic.serialize().size());
    senderKeyStateStructure.mutable_sendersigningkey()->set_private_(signatureKeyPrivate.serialize().c_str(),
                                                                     signatureKeyPrivate.serialize().size());

    /*textsecure::SenderKeyStateStructure::SenderChainKey senderChainKeyStructure;
    senderChainKeyStructure.set_iteration(iteration);
    senderChainKeyStructure.set_seed(chainKey.c_str());

    textsecure::SenderKeyStateStructure::SenderSigningKey signingKeyStructure;
    signingKeyStructure.set_public_(signatureKeyPublic.serialize().c_str());

    signingKeyStructure.set_private_(signatureKeyPrivate.serialize().c_str());

    senderKeyStateStructure = textsecure::SenderKeyStateStructure();
    senderKeyStateStructure.set_senderkeyid(id);
    senderKeyStateStructure.mutable_senderchainkey()->CopyFrom(senderChainKeyStructure);
    senderKeyStateStructure.mutable_sendersigningkey()->CopyFrom(signingKeyStructure);*/
}

SenderKeyState::SenderKeyState(const textsecure::SenderKeyStateStructure &senderKeyStateStructure)
{
    this->senderKeyStateStructure = senderKeyStateStructure;
}

int SenderKeyState::getKeyId() const
{
    return senderKeyStateStructure.senderkeyid();
}

SenderChainKey SenderKeyState::getSenderChainKey() const
{
    ::std::string seed = senderKeyStateStructure.senderchainkey().seed();
    return SenderChainKey(senderKeyStateStructure.senderchainkey().iteration(),
                          ByteArray(seed.data(), seed.length()));
}

void SenderKeyState::setSenderChainKey(const SenderChainKey &chainKey)
{
    senderKeyStateStructure.mutable_senderchainkey()->set_iteration(chainKey.getIteration());
    senderKeyStateStructure.mutable_senderchainkey()->set_seed(chainKey.getSeed().c_str(),
                                                               chainKey.getSeed().size());

    /*textsecure::SenderKeyStateStructure::SenderChainKey senderChainKeyStructure;
    senderChainKeyStructure.set_iteration(chainKey.getIteration());
    senderChainKeyStructure.set_seed(chainKey.getSeed().c_str());

    senderKeyStateStructure.mutable_senderchainkey()->CopyFrom(senderChainKeyStructure);*/
}

DjbECPublicKey SenderKeyState::getSigningKeyPublic() const
{
    ::std::string sendersigningkeypublic = senderKeyStateStructure.sendersigningkey().public_();
    return Curve::decodePoint(ByteArray(sendersigningkeypublic.data(), sendersigningkeypublic.length()), 0);
}

DjbECPrivateKey SenderKeyState::getSigningKeyPrivate() const
{
    ::std::string sendersigningkeyprivate = senderKeyStateStructure.sendersigningkey().public_();
    return Curve::decodePrivatePoint(ByteArray(sendersigningkeyprivate.data(), sendersigningkeyprivate.length()));
}

bool SenderKeyState::hasSenderMessageKey(uint32_t iteration) const
{
    for (int i = 0; i < senderKeyStateStructure.sendermessagekeys_size(); i++) {
        textsecure::SenderKeyStateStructure::SenderMessageKey senderMessageKey = senderKeyStateStructure.sendermessagekeys(i);
        if (senderMessageKey.iteration() == iteration) {
            return true;
        }
    }

    return false;
}

void SenderKeyState::addSenderMessageKey(const SenderMessageKey &senderMessageKey)
{
    senderKeyStateStructure.add_sendermessagekeys()->set_iteration(senderMessageKey.getIteration());
    senderKeyStateStructure.add_sendermessagekeys()->set_seed(senderMessageKey.getSeed().c_str(),
                                                              senderMessageKey.getSeed().size());

    /*textsecure::SenderKeyStateStructure::SenderMessageKey senderMessageKeyStructure;
    senderMessageKeyStructure.set_iteration(senderMessageKey.getIteration());
    senderMessageKeyStructure.set_seed(senderMessageKey.getSeed().c_str());

    senderKeyStateStructure.add_sendermessagekeys()->CopyFrom(senderMessageKeyStructure);*/
}

SenderMessageKey SenderKeyState::removeSenderMessageKey(uint32_t iteration)
{
    SenderMessageKey result;
    for (int i = 0; i < senderKeyStateStructure.sendermessagekeys_size(); i++) {
        textsecure::SenderKeyStateStructure::SenderMessageKey *senderMessageKey = senderKeyStateStructure.mutable_sendermessagekeys(i);
        if (senderMessageKey->iteration() == iteration) {
            ::std::string senderMessageKeySeed = senderMessageKey->seed();
            result = SenderMessageKey(iteration, ByteArray(senderMessageKeySeed.data(), senderMessageKeySeed.length()));
            delete senderMessageKey;
            break;
        }
    }

    return result;
}

textsecure::SenderKeyStateStructure SenderKeyState::getStructure() const
{
    return senderKeyStateStructure;
}
