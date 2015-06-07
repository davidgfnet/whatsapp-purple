
#ifndef __WACONNECTION__H__
#define __WACONNECTION__H__

#include <string>
#include <vector>
#include <map>
#include <time.h>
#include "wacommon.h"
#include "databuffer.h"
#include "contacts.h"

class ChatMessage;
class ImageMessage;
class Message;
class RC4Decoder;
class Tree;

struct t_fileupload {
	std::string to, from;
	std::string file, hash;
	int rid;
	std::string type;
	std::string uploadurl, host, ip;
	std::string thumbnail;
	std::string msgid;
	bool uploading;
	int totalsize;
};

enum ReceptionType { rSent, rDelivered, rRead };
struct t_message_reception {
	std::string id;
	ReceptionType type;
	unsigned long long t;
	std::string from;
};

class WhatsappConnection {
	friend class ChatMessage;
	friend class ImageMessage;
	friend class Message;
	friend class CallMessage;
	friend class VCardMessage;

public:
	enum ErrorCode { errorNoError = 0, errorAuth, errorUnknown };

private:

	enum SessionStatus { SessionNone = 0, SessionConnecting = 1, SessionWaitingChallenge = 2, SessionWaitingAuthOK = 3, SessionConnected = 4 };

	/* Current dissection classes */
	RC4Decoder * in, *out;
	unsigned char session_key[20*4]; // V1.4 update
	unsigned int frame_seq;
	DataBuffer inbuffer, outbuffer;
	DataBuffer sslbuffer, sslbuffer_in;
	std::string challenge_data, challenge_response;
	std::string phone, password;
	SessionStatus conn_status;
	time_t last_keepalive;

	/* State stuff */
	unsigned int msgcounter, iqid;
	std::string nickname;
	std::string whatsappserver, whatsappservergroup;
	std::string mypresence, mymessage;
	bool sendRead;
	std::vector < std::pair<ErrorCode,std::string> > error_queue;

	/* Various account info */
	std::string account_type, account_status, account_expiration, account_creation;

	/* Privacy */
	std::string show_last_seen, show_profile_pic, show_status_msg;

	/* Groups stuff */
	std::map < std::string, Group > groups;
	bool groups_updated;

	/* Blist stuff */
	std::map < std::string, BList > blists;
	bool blists_updated;

	/* Contacts & msg */
	std::map < std::string, Contact > contacts;
	std::vector < Message * >recv_messages;
	std::vector < std::string > user_changes, user_icons, user_typing;

	/* Reception queue */
	std::vector < t_message_reception > received_messages;

	void processIncomingData();
	void processSSLIncomingData();
	DataBuffer serialize_tree(Tree * tree, bool crypt = true);
	DataBuffer write_tree(Tree * tree);
	bool parse_tree(DataBuffer * data, Tree & t);

	/* Upload */
	std::vector < t_fileupload > uploadfile_queue;

	/* SSL / HTTPS interface */
	int sslstatus;		/* 0 Idle, 1 sending request, 2 getting response */
	/* 5/6 for image upload */

	void receiveMessage(const Message & m);
	void notifyPresence(std::string from, std::string presence);
	void updatePrivacy();

	void notifyLastSeen(std::string from, std::string seconds);
	void addPreviewPicture(std::string from, std::string picture);
	void addFullsizePicture(std::string from, std::string picture);
	void sendResponse();
	void doPong(std::string id, std::string from);
	void subscribePresence(std::string user);
	void getLast(std::string user);
	void queryPreview(std::string user);
	void queryFullSize(std::string user);
	void gotTyping(std::string who, std::string tstat);
	void updateGroups();
	void updateBlists();
	void queryStatuses();

	void notifyMyMessage();
	void notifyMyPresence();
	void sendInitial();
	void notifyError(ErrorCode err, const std::string & reason);
	DataBuffer generateResponse(std::string from, std::string type, std::string id);
	std::string generateUploadPOST(t_fileupload * fu);
	void processUploadQueue();

	void updateContactStatuses(std::string json);
	void updateFileUpload(std::string);

	std::string getNextIqId();
	std::string tohex(int);

public:
	bool read_tree(DataBuffer * data, Tree & tt);

	WhatsappConnection(std::string phone, std::string password, std::string nick);
	~WhatsappConnection();

	std::string getPhone() const { return phone; }

	void doLogin(std::string);
	void receiveCallback(const char *data, int len);
	int sendCallback(char *data, int len);
	void sentCallback(int len);
	bool hasDataToSend();

	ErrorCode getErrors(std::string & reason);

	Message * getReceivedMessage();
	bool queryReceivedMessage(std::string & msgid, int & type, unsigned long long & t, std::string & sender);

	void updatePrivacy(const std::string &, const std::string &, const std::string &);
	void queryPrivacy(std::string &, std::string &, std::string &);

	std::string getMessageId();
	void addContacts(std::vector < std::string > clist);
	void contactsUpdate();
	void sendChat(std::string msgid, std::string to, std::string message);
	void sendGroupChat(std::string msgid, std::string to, std::string message);
	bool query_status(std::string & from, int &status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	bool query_avatar(std::string user, std::string & icon);
	bool query_typing(std::string & from, int &status);
	void send_avatar(const std::string & avatar, const std::string & avatarp);
	void account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);

	void manageParticipant(std::string group, std::string participant, std::string command);
	void leaveGroup(std::string group);
	void deleteBlist(std::string id);

	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);
	std::map < std::string, Group > getGroups();
	bool groupsUpdated();
	bool blistsUpdated();
	void addGroup(std::string subject);

	int loginStatus() const
	{
		return ((int)conn_status) - 1;
	}
	int sendImage(std::string mid, std::string to, int w, int h, unsigned int size, const char *fp);
	void sendVCard(const std::string msgid, const std::string to, const std::string name, const std::string vcard);

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

