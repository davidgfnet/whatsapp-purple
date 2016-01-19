
#ifndef GROUP_SESSION_BUILDER_H_
#define GROUP_SESSION_BUILDER_H_

#include <string>
#include <memory>
#include "state/axolotlstore.h"

class GroupSessionBuilder {
public:

	GroupSessionBuilder(std::shared_ptr<AxolotlStore> store);

	void process(std::string groupId, std::string senderKeyDistMsg);

private:
	std::shared_ptr<AxolotlStore> senderKeyStore;

};

#endif

