
#ifndef AXOLOTL_GROUPS_H_
#define AXOLOTL_GROUPS_H_

#include "protocol/senderkeymessage.h"
#include "invalidmessageexception.h"
#include "invalidkeyexception.h"
#include "duplicatemessageexception.h"

#include "aes.h"


class GroupCipher {
private:
	std::shared_ptr<AxolotlStore> senderKeyStore;
	std::string senderKeyId;

public:
	GroupCipher(std::shared_ptr<AxolotlStore> senderKeyStore, std::string senderKeyId)
		: senderKeyStore(senderKeyStore), senderKeyId(senderKeyId)
	{ }


	std::string unpad(std::string s) {
		unsigned int pl = *(unsigned char*)&s[s.size() - 1];
		return std::string(s.c_str(), s.size() - pl);
	}

    std::string getPlainText(std::string iv, std::string key, std::string ciphertext) {
		AES_KEY dec_key;
		char outb[ciphertext.size()];
		AES_set_decrypt_key((const unsigned char*)key.c_str(), key.size() * 8, &dec_key);
        AES_cbc_encrypt((const unsigned char*)ciphertext.c_str(),
                        (unsigned char*)outb,
                        ciphertext.size(), &dec_key,
                        (unsigned char*)iv.data(), AES_DECRYPT);

		std::string out(outb, ciphertext.size());
		return unpad(unpad(out));
    }

    std::string getCipherText(std::string iv, std::string key, std::string plaintext) {
		// Add two paddings? No freaking idea man
		unsigned int padsize = 16 - (plaintext.size() % 16);
		for (unsigned i = 0; i < padsize; i++)
			plaintext += (char)padsize;
		for (unsigned i = 0; i < 16; i++)
			plaintext += (char)16;
		
		AES_KEY enc_key;
		char outb[plaintext.size()];
		AES_set_encrypt_key((const unsigned char*)key.c_str(), key.size() * 8, &enc_key);
        AES_cbc_encrypt((const unsigned char*)plaintext.c_str(),
                        (unsigned char*)outb,
                        plaintext.size(), &enc_key,
                        (unsigned char*)iv.data(), AES_ENCRYPT);

		return std::string(outb, plaintext.size());
    }

	std::string encrypt(std::string paddedPlaintext) {
		SenderKeyRecord record = senderKeyStore->loadSenderKey(this->senderKeyId);
		SenderKeyState * senderKeyState = record.getSenderKeyState();
		SenderMessageKey senderKey = senderKeyState->getSenderChainKey().getSenderMessageKey();

		std::string ciphertext = getCipherText(
			senderKey.getIv(), senderKey.getCipherKey(), paddedPlaintext);

		SenderKeyMessage senderKeyMessage(senderKeyState->getKeyId(), senderKey.getIteration(), ciphertext, senderKeyState->getSigningKeyPrivate());


		senderKeyState->setSenderChainKey(senderKeyState->getSenderChainKey().getNext());
		senderKeyStore->storeSenderKey(this->senderKeyId, &record);

		return senderKeyMessage.serialize();
	}

	std::string decrypt(const std::string senderKeyMessageBytes) {
		SenderKeyRecord record = senderKeyStore->loadSenderKey(this->senderKeyId);
		SenderKeyMessage * senderKeyMessage = new SenderKeyMessage(senderKeyMessageBytes);

		SenderKeyState * senderKeyState = record.getSenderKeyState(senderKeyMessage->getKeyId());

		senderKeyMessage->verifySignature(senderKeyState->getSigningKeyPublic());
		SenderMessageKey senderKey = this->getSenderKey(senderKeyState, senderKeyMessage->getIteration());

		std::string plaintext = getPlainText(
			senderKey.getIv(), senderKey.getCipherKey(), senderKeyMessage->getCipherText());

		this->senderKeyStore->storeSenderKey(this->senderKeyId, &record);
		return plaintext;
    }

    SenderMessageKey getSenderKey(SenderKeyState * senderKeyState, int iteration) {
        SenderChainKey senderChainKey = senderKeyState->getSenderChainKey();

        if (senderChainKey.getIteration() > iteration) {
            if (senderKeyState->hasSenderMessageKey(iteration))
                return senderKeyState->removeSenderMessageKey(iteration);
            else
                throw DuplicateMessageException ("Received message with old counter: ");
        }

        if (senderChainKey.getIteration() - iteration > 2000)
            throw InvalidMessageException ("Over 2000 messages into the future!");

        while (senderChainKey.getIteration() < iteration) {
            senderKeyState->addSenderMessageKey(senderChainKey.getSenderMessageKey());
            senderChainKey = senderChainKey.getNext();
        }

        senderKeyState->setSenderChainKey(senderChainKey.getNext());
        return senderChainKey.getSenderMessageKey();
    }

};

#endif


