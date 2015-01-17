
#ifndef __WHATSAPP_API__H__
#define __WHATSAPP_API__H__

#include <string>
#include <vector>
#include <map>

class WhatsappConnection;
class Group;

class WhatsappConnectionAPI {
private:
	WhatsappConnection * connection;

public:
	WhatsappConnectionAPI(std::string phone, std::string password, std::string nick);
	~WhatsappConnectionAPI();

	void doLogin(std::string);
	void receiveCallback(const char *data, int len);
	int sendCallback(char *data, int len);
	void sentCallback(int len);
	bool hasDataToSend();

	bool queryReceivedMessage(char *msgid, int * type);
	void getMessageId(char * msgid);
	void addContacts(std::vector < std::string > clist);
	void sendChat(std::string msgid, std::string to, std::string message);
	void sendGroupChat(std::string msgid, std::string to, std::string message);
	bool query_chat(std::string & from, std::string & message, std::string & author, unsigned long &t);
	bool query_chatimages(std::string & from, std::string & preview, std::string & url, std::string & author, unsigned long &t);
	bool query_chatsounds(std::string & from, std::string & url, std::string & author, unsigned long &t);
	bool query_chatvideos(std::string & from, std::string & url, std::string & author, unsigned long &t);
	bool query_chatlocations(std::string & from, double &lat, double &lng, std::string & prev, std::string & author, unsigned long &t);
	bool query_status(std::string & from, int &status);
	int query_next();
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	bool query_avatar(std::string user, std::string & icon);
	bool query_typing(std::string & from, int &status);
	void account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status);
	void send_avatar(const std::string & avatar);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);
	void addGroup(std::string subject);
	void leaveGroup(std::string group);
	void manageParticipant(std::string group, std::string participant, std::string command);

	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);

	std::map < std::string, Group > getGroups();
	bool groupsUpdated();

	int loginStatus() const;

	int sendImage(std::string to, int w, int h, unsigned int size, const char *fp);

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


