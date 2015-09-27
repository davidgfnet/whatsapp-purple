
#ifndef __CONTACTS__H__
#define __CONTACTS__H__

#include <vector>
#include <string>

class Group {
public:
	class Participant {
	public:
		Participant(std::string p, std::string t)
		: jid(p), type(t) {}
		std::string jid, type;
	};
	Group(std::string id, 
		std::string subject, unsigned long long subject_time,
		std::string owner, std::string creator,
		unsigned long long creation_time)
	: id(id), subject(subject), owner(owner), creator(creator),
	creation_time(creation_time), subject_time(subject_time) {}

	std::string id, subject, owner, creator;
	std::vector < Participant > participants;
	unsigned long long creation_time, subject_time;

	std::string getAdminList() const {
		std::string ret;
		for (auto p : participants)
			if (p.type == "admin")
				ret = "," + p.jid;
		if (ret.size())
			ret = ret.substr(1);
		return ret;
	}
	std::string getParticipantsList() const {
		std::string ret;
		for (auto p : participants)
			ret = "," + p.jid;
		if (ret.size())
			ret = ret.substr(1);
		return ret;
	}
};

class BList {
public:
	BList(std::string id, std::string name)
	: id(id), name(name) {}
	std::string id, name;
	std::vector < std::string > dests;
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

