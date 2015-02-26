
#include "whatsapp_api.h"
#include "wa_connection.h"

WhatsappConnectionAPI::WhatsappConnectionAPI(std::string phone, std::string password, std::string nick)
{
	connection = new WhatsappConnection(phone, password, nick);
}

WhatsappConnectionAPI::~WhatsappConnectionAPI()
{
	delete connection;
}

std::map < std::string, Group > WhatsappConnectionAPI::getGroups()
{
	return connection->getGroups();
}

bool WhatsappConnectionAPI::groupsUpdated()
{
	return connection->groupsUpdated();
}

int WhatsappConnectionAPI::getuserstatus(const std::string & who)
{
	return connection->getuserstatus(who);
}

void WhatsappConnectionAPI::addGroup(std::string subject)
{
	connection->addGroup(subject);
}

void WhatsappConnectionAPI::leaveGroup(std::string subject)
{
	connection->leaveGroup(subject);
}

void WhatsappConnectionAPI::manageParticipant(std::string group, std::string participant, std::string command)
{
	connection->manageParticipant(group, participant, command);
}

unsigned long long WhatsappConnectionAPI::getlastseen(const std::string & who)
{
	return connection->getlastseen(who);
}

int WhatsappConnectionAPI::sendImage(std::string to, int w, int h, unsigned int size, const char *fp)
{
	return connection->sendImage(to, w, h, size, fp);
}

int WhatsappConnectionAPI::uploadProgress(int &rid, int &bs)
{
	return connection->uploadProgress(rid, bs);
}
int WhatsappConnectionAPI::uploadComplete(int rid)
{
	return connection->uploadComplete(rid);
}

void WhatsappConnectionAPI::send_avatar(const std::string & avatar)
{
	connection->send_avatar(avatar);
}

bool WhatsappConnectionAPI::query_icon(std::string & from, std::string & icon, std::string & hash)
{
	return connection->query_icon(from, icon, hash);
}

bool WhatsappConnectionAPI::query_avatar(std::string user, std::string & icon)
{
	return connection->query_avatar(user, icon);
}

bool WhatsappConnectionAPI::query_typing(std::string & from, int &status)
{
	return connection->query_typing(from, status);
}

void WhatsappConnectionAPI::setMyPresence(std::string s, std::string msg)
{
	connection->setMyPresence(s, msg);
}

void WhatsappConnectionAPI::notifyTyping(std::string who, int status)
{
	connection->notifyTyping(who, status);
}

std::string WhatsappConnectionAPI::getuserstatusstring(const std::string & who)
{
	return connection->getuserstatusstring(who);
}

bool WhatsappConnectionAPI::query_status(std::string & from, int &status)
{
	return connection->query_status(from, status);
}

void WhatsappConnectionAPI::getMessageId(char * msgid)
{
	connection->getMessageId(msgid);
}

bool WhatsappConnectionAPI::queryReceivedMessage(char * msgid, int * type)
{
	return connection->queryReceivedMessage(msgid, type);
}

void WhatsappConnectionAPI::sendChat(std::string msgid, std::string to, std::string message)
{
	connection->sendChat(msgid, to, message);
}

void WhatsappConnectionAPI::sendGroupChat(std::string msgid, std::string to, std::string message)
{
	connection->sendGroupChat(msgid, to, message);
}

int WhatsappConnectionAPI::loginStatus() const
{
	return connection->loginStatus();
}

void WhatsappConnectionAPI::doLogin(std::string resource)
{
	connection->doLogin(resource);
}

void WhatsappConnectionAPI::receiveCallback(const char *data, int len)
{
	connection->receiveCallback(data, len);
}

int WhatsappConnectionAPI::sendCallback(char *data, int len)
{
	return connection->sendCallback(data, len);
}

void WhatsappConnectionAPI::sentCallback(int len)
{
	connection->sentCallback(len);
}

void WhatsappConnectionAPI::addContacts(std::vector < std::string > clist)
{
	connection->addContacts(clist);
}

bool WhatsappConnectionAPI::hasDataToSend()
{
	return connection->hasDataToSend();
}

void WhatsappConnectionAPI::account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status)
{
	connection->account_info(creation, freeexp, status);
}

int WhatsappConnectionAPI::sendSSLCallback(char *buffer, int maxbytes)
{
	return connection->sendSSLCallback(buffer, maxbytes);
}

int WhatsappConnectionAPI::sentSSLCallback(int bytessent)
{
	return connection->sentSSLCallback(bytessent);
}

void WhatsappConnectionAPI::receiveSSLCallback(char *buffer, int bytesrecv)
{
	connection->receiveSSLCallback(buffer, bytesrecv);
}

bool WhatsappConnectionAPI::hasSSLDataToSend()
{
	return connection->hasSSLDataToSend();
}

bool WhatsappConnectionAPI::closeSSLConnection()
{
	return connection->closeSSLConnection();
}

void WhatsappConnectionAPI::SSLCloseCallback()
{
	connection->SSLCloseCallback();
}

bool WhatsappConnectionAPI::hasSSLConnection(std::string & host, int *port)
{
	return connection->hasSSLConnection(host, port);
}

Message * WhatsappConnectionAPI::getReceivedMessage() {
	return connection->getReceivedMessage();
}

void WhatsappConnectionAPI::updatePrivacy(const std::string & a, const std::string & b, const std::string & c) {
	connection->updatePrivacy(a,b,c);
}

void WhatsappConnectionAPI::queryPrivacy(std::string & a, std::string & b, std::string & c) {
	connection->queryPrivacy(a,b,c);
}

int WhatsappConnectionAPI::getErrors(std::string & reason) {
	return (int)connection->getErrors(reason);
}


