
#ifndef AXOLOTL_GROUPS_H_
#define AXOLOTL_GROUPS_H_

#include "protocol/senderkeymessage.h"
#include "invalidmessageexception.h"
#include "invalidkeyexception.h"
#include "duplicatemessageexception.h"

class GroupCipher {
private:
	std::shared_ptr<AxolotlStore> senderKeyStore;
	std::string senderKeyId;

public:
	GroupCipher(std::shared_ptr<AxolotlStore> senderKeyStore, std::string senderKeyId)
		: senderKeyStore(senderKeyStore), senderKeyId(senderKeyId)
	{ }

	/*
	std::string encrypt(std::string paddedPlaintext) {
        try{

            $record         = $this->senderKeyStore->loadSenderKey($this->senderKeyId);
            $senderKeyState = $record->getSenderKeyState();
            $senderKey      = $senderKeyState->getSenderChainKey()->getSenderMessageKey();
            $ciphertext     = $this->getCipherText($senderKey->getIv(), $senderKey->getCipherKey(), $paddedPlaintext);

            $senderKeyMessage = new SenderKeyMessage($senderKeyState->getKeyId(),
                                                                 $senderKey->getIteration(),
                                                                 $ciphertext,
                                                                 $senderKeyState->getSigningKeyPrivate());

            $senderKeyState->setSenderChainKey($senderKeyState->getSenderChainKey()->getNext());
            $this->senderKeyStore->storeSenderKey($this->senderKeyId, $record);

            return $senderKeyMessage->serialize();
        } catch (InvalidKeyIdException $e)
        {

            throw new NoSessionException($e->getMessage());
        }
    }
	*/

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
                                              //$senderChainKey->getIteration() . " " .
                                              //$iteration);
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


    std::string getPlainText(std::string iv, std::string key, std::string ciphertext) {
        /*try
        {
            $cipher = new AESCipher($key, $iv);
            $plaintext = $cipher->decrypt($ciphertext);
            return $plaintext;
        } catch (Exception $e)
        {
          throw new InvalidMessageException($e->getMessage());
        }*/
		return "FIXME DUMMY TEXT";
    }

    std::string getCipherText(std::string iv, std::string key, std::string plaintext) {
        //$cipher = new AESCipher($key, $iv);
        //return $cipher->encrypt($plaintext);
    }
};

#endif


