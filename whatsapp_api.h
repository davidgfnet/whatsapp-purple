
#ifndef __WHATSAPP_API__H__
#define __WHATSAPP_API__H__

#include <string>
#include <vector>
#include <map>

class WhatsappConnection;
class Group;
class Message;

class WhatsappConnectionAPI {
private:
	WhatsappConnection * connection;

public:
	WhatsappConnectionAPI(std::string phone, std::string password, std::string nick);
	~WhatsappConnectionAPI();

	// Login/Auth functions
	void doLogin(std::string);
	int loginStatus() const;
	int getErrors(std::string & reason);

	// Data transfer
	void receiveCallback(const char *data, int len);
	int sendCallback(char *data, int len);
	void sentCallback(int len);
	bool hasDataToSend();

	// Receiving stuff
	bool queryReceivedMessage(char *msgid, int * type);
	Message * getReceivedMessage();

	// Sending stuff
	void getMessageId(char * msgid);
	void sendChat(std::string msgid, std::string to, std::string message);
	void sendGroupChat(std::string msgid, std::string to, std::string message);
	int sendImage(std::string to, int w, int h, unsigned int size, const char *fp);

	// Privacy
	void updatePrivacy(const std::string &, const std::string &, const std::string &);
	void queryPrivacy(std::string &, std::string &, std::string &);

	void addContacts(std::vector < std::string > clist);
	void contactsUpdate();
	bool query_status(std::string & from, int &status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	bool query_avatar(std::string user, std::string & icon);
	bool query_typing(std::string & from, int &status);
	void account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status);
	void send_avatar(const std::string & avatar);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);

	// Group functionality
	void addGroup(std::string subject);
	void leaveGroup(std::string group);
	void manageParticipant(std::string group, std::string participant, std::string command);
	std::map < std::string, Group > getGroups();
	bool groupsUpdated();

	// User
	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);

	// HTTP/SSL interface for file upload and so...
	int sendSSLCallback(char *buffer, int maxbytes);
	int sentSSLCallback(int bytessent);
	void receiveSSLCallback(char *buffer, int bytesrecv);
	bool hasSSLDataToSend();
	bool closeSSLConnection();
	void SSLCloseCallback();
	bool hasSSLConnection(std::string & host, int *port);
	int uploadProgress(int &rid, int &bs);
	int uploadComplete(int);
};


#endif


