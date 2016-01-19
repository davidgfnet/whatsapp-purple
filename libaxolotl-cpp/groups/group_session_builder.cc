
#include "group_session_builder.h"
#include "WhisperTextProtocol.pb.h"

#include <iostream>

GroupSessionBuilder::GroupSessionBuilder(std::shared_ptr<AxolotlStore> store)
 :senderKeyStore(store)
{
	
}

void GroupSessionBuilder::process(std::string groupId, std::string senderKeyDistMsg) {
	textsecure::SenderKeyDistributionMessage skdm;
	skdm.ParseFromString(senderKeyDistMsg.substr(1));

	SenderKeyRecord skr = senderKeyStore->loadSenderKey(groupId);
	skr.addSenderKeyState(skdm.id(), skdm.iteration(),
		skdm.chainkey(), skdm.signingkey());
	senderKeyStore->storeSenderKey(groupId, &skr);
}


