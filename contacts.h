
#ifndef __CONTACTS__H__
#define __CONTACTS__H__

#include <vector>
#include <string>

class Group {
public:
	Group(std::string id, std::string subject, std::string owner)
	{
		this->id = id;
		this->subject = subject;
		this->owner = owner;
	}
	~Group() {
	}
	std::string id, subject, owner;
	std::vector < std::string > participants;
};

class Contact {
public:
	Contact()
	{
	}
	Contact(std::string phone, bool myc)
	{
		this->phone = phone;
		this->mycontact = myc;
		this->last_seen = 0;
		this->subscribed = false;
		this->typing = "paused";
		this->status = "";
	}

	std::string phone, name;
	std::string presence, typing;
	std::string status;
	unsigned long long last_seen, last_status;
	bool mycontact;
	std::string ppprev, pppicture;
	bool subscribed;
};

#endif

