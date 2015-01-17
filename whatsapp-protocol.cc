
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 * v1.4 changes based on WP7 sources
 *
 * Share and enjoy!
 *
 */

#include <iostream>
#include <map>
#include <vector>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#ifdef ENABLE_OPENSSL
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#else
#include "wa_api.h"
#endif

#include "wadict.h"
#include "rc4.h"
#include "keygen.h"
#include "databuffer.h"
#include "tree.h"
#include "contacts.h"
#include "message.h"
#include "wa_connection.h"
#include "thumb.h"

DataBuffer WhatsappConnection::generateResponse(std::string from, std::string type, std::string id)
{
	if (type == "") { // Auto 
		if (sendRead) type = "read";
		else type = "delivery";
	}
	Tree mes("receipt", makeAttr4("to", from, "id", id, "type", type, "t", int2str(1)));

	return serialize_tree(&mes);
}

/* Send image transaction */
int WhatsappConnection::sendImage(std::string to, int w, int h, unsigned int size, const char *fp)
{
	/* Type can be: audio/image/video */
	std::string sha256b64hash = SHA256_file_b64(fp);
	Tree iq("media", makeAttr3("type", "image", "hash", sha256b64hash, "size", int2str(size)));
	Tree req("iq", makeAttr4("id", int2str(++iqid), "type", "set", "to", whatsappserver, "xmlns", "w:m"));
	req.addChild(iq);

	t_fileupload fu;
	fu.to = to;
	fu.file = std::string(fp);
	fu.rid = iqid;
	fu.hash = sha256b64hash;
	fu.type = "image";
	fu.uploading = false;
	fu.totalsize = 0;
	uploadfile_queue.push_back(fu);
	outbuffer = outbuffer + serialize_tree(&req);

	return iqid;
}

WhatsappConnection::WhatsappConnection(std::string phone, std::string password, std::string nickname)
{
	this->phone = phone;
	this->password = password;
	this->in = NULL;
	this->out = NULL;
	this->conn_status = SessionNone;
	this->msgcounter = 1;
	this->iqid = 1;
	this->nickname = nickname;
	this->whatsappserver = "s.whatsapp.net";
	this->whatsappservergroup = "g.us";
	this->mypresence = "available";
	this->groups_updated = false;
	this->gq_stat = 0;
	this->gw1 = -1;
	this->gw2 = -1;
	this->gw3 = 0;
	this->sslstatus = 0;
	this->frame_seq = 0;
	this->sendRead = true;

	/* Trim password spaces */
	while (password.size() > 0 and password[0] == ' ')
		password = password.substr(1);
	while (password.size() > 0 and password[password.size() - 1] == ' ')
		password = password.substr(0, password.size() - 1);
}

WhatsappConnection::~WhatsappConnection()
{
	if (this->in)
		delete this->in;
	if (this->out)
		delete this->out;
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		delete recv_messages[i];
	}
}

std::map < std::string, Group > WhatsappConnection::getGroups()
{
	return groups;
}

bool WhatsappConnection::groupsUpdated()
{
	if (gq_stat == 7) {
		groups_updated = true;
		gq_stat = 8;
	}

	if (groups_updated and gw3 <= 0) {
		groups_updated = false;
		return true;
	}

	return false;
}

void WhatsappConnection::updateGroups()
{
	/* Get the group list */
	groups.clear();
	{
		gw1 = iqid;
		Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", "g.us", "xmlns", "w:g"));
		req.addChild(Tree("list", makeAttr1("type", "owning")));
		outbuffer = outbuffer + serialize_tree(&req);
	}
	{
		gw2 = iqid;
		Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", "g.us", "xmlns", "w:g"));
		req.addChild(Tree("list", makeAttr1("type", "participating")));
		outbuffer = outbuffer + serialize_tree(&req);
	}
	gq_stat = 1;		/* Queried the groups */
	gw3 = 0;
}

void WhatsappConnection::manageParticipant(std::string group, std::string participant, std::string command)
{
	Tree part("participant", makeAttr1("jid", participant));
	Tree iq(command);
	iq.addChild(part);
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "set", "to", group + "@g.us", "xmlns", "w:g"));
	req.addChild(iq);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::leaveGroup(std::string group)
{
	Tree gr("group", makeAttr1("id", group + "@g.us"));
	Tree iq("leave");
	iq.addChild(gr);
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "set", "to", "g.us", "xmlns", "w:g"));
	req.addChild(iq);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::addGroup(std::string subject)
{
	Tree gr("group", makeAttr2("action", "create", "subject", subject));
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "set", "to", "g.us", "xmlns", "w:g"));
	req.addChild(gr);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::doLogin(std::string resource)
{
	/* Send stream init */
	DataBuffer first;

	{
		first.addData("WA\1\5", 4);
		Tree t("start", makeAttr2("resource",resource, "to",whatsappserver));
		first = first + serialize_tree(&t, false);
	}

	/* Send features */
	{
		Tree p("stream:features");
		p.addChild(Tree("readreceipts"));
		first = first + serialize_tree(&p, false);
	}

	/* Send auth request */
	{
		Tree t("auth", makeAttr2("mechanism","WAUTH-2", "user",phone));
		t.forceDataWrite();
		first = first + serialize_tree(&t, false);
	}

	conn_status = SessionWaitingChallenge;
	outbuffer = first;
}

void WhatsappConnection::receiveCallback(const char *data, int len)
{
	if (data != NULL and len > 0)
		inbuffer.addData(data, len);
	this->processIncomingData();
}

int WhatsappConnection::sendCallback(char *data, int len)
{
	int minlen = outbuffer.size();
	if (minlen > len)
		minlen = len;

	memcpy(data, outbuffer.getPtr(), minlen);
	return minlen;
}

bool WhatsappConnection::hasDataToSend()
{
	return outbuffer.size() != 0;
}

void WhatsappConnection::sentCallback(int len)
{
	outbuffer.popData(len);
}

int WhatsappConnection::sendSSLCallback(char *buffer, int maxbytes)
{
	int minlen = sslbuffer.size();
	if (minlen > maxbytes)
		minlen = maxbytes;

	memcpy(buffer, sslbuffer.getPtr(), minlen);
	return minlen;
}

int WhatsappConnection::sentSSLCallback(int bytessent)
{
	sslbuffer.popData(bytessent);
	return bytessent;
}

void WhatsappConnection::receiveSSLCallback(char *buffer, int bytesrecv)
{
	if (buffer != NULL and bytesrecv > 0)
		sslbuffer_in.addData(buffer, bytesrecv);
	this->processSSLIncomingData();
}

bool WhatsappConnection::hasSSLDataToSend()
{
	return sslbuffer.size() != 0;
}

bool WhatsappConnection::closeSSLConnection()
{
	return sslstatus == 0;
}

void WhatsappConnection::SSLCloseCallback()
{
	sslstatus = 0;
}

bool WhatsappConnection::hasSSLConnection(std::string & host, int *port)
{
	host = "";
	*port = 443;

	if (sslstatus == 1)
		for (unsigned int j = 0; j < uploadfile_queue.size(); j++)
			if (uploadfile_queue[j].uploading) {
				host = uploadfile_queue[j].host;
				return true;
			}

	return false;
}

int WhatsappConnection::uploadProgress(int &rid, int &bs)
{
	if (!(sslstatus == 1 or sslstatus == 2))
		return 0;
	int totalsize = 0;
	for (unsigned int j = 0; j < uploadfile_queue.size(); j++)
		if (uploadfile_queue[j].uploading) {
			rid = uploadfile_queue[j].rid;
			totalsize = uploadfile_queue[j].totalsize;
			break;
		}
	bs = totalsize - sslbuffer.size();
	if (bs < 0)
		bs = 0;
	return 1;
}

int WhatsappConnection::uploadComplete(int rid) {
	for (unsigned int j = 0; j < uploadfile_queue.size(); j++)
		if (rid == uploadfile_queue[j].rid)
			return 0;

	return 1;
}

void WhatsappConnection::subscribePresence(std::string user)
{
	Tree request("presence", makeAttr2("type", "subscribe", "to", user));
	outbuffer = outbuffer + serialize_tree(&request);
}

void WhatsappConnection::queryStatuses()
{
	Tree req("iq", makeAttr4("to", "s.whatsapp.net", "type", "get", "id", int2str(iqid++), "xmlns", "status"));
	Tree stat("status");

	for (std::map < std::string, Contact >::iterator iter = contacts.begin(); iter != contacts.end(); iter++)
	{
		stat.addChild(Tree("user", makeAttr1("jid", iter->first + "@" + whatsappserver)));
	}
	req.addChild(stat);
	
	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::getLast(std::string user)
{
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", user, "xmlns", "jabber:iq:last"));
	req.addChild(Tree("query"));

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::gotTyping(std::string who, std::string tstat)
{
	who = getusername(who);
	if (contacts.find(who) != contacts.end()) {
		contacts[who].typing = tstat;
		user_typing.push_back(who);
	}
}

void WhatsappConnection::notifyTyping(std::string who, int status)
{
	std::string s = "paused";
	if (status == 1)
		s = "composing";

	Tree mes("chatstate", makeAttr1("to", who + "@" + whatsappserver));
	mes.addChild(Tree(s));

	outbuffer = outbuffer + serialize_tree(&mes);
}

void WhatsappConnection::account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status)
{
	creation = str2lng(account_creation);
	freeexp = str2lng(account_expiration);
	status = account_status;
}

void WhatsappConnection::queryPreview(std::string user)
{
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", user, "xmlns", "w:profile:picture"));
	req.addChild(Tree("picture", makeAttr1("type", "preview")));

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::queryFullSize(std::string user)
{
	Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", user, "xmlns", "w:profile:picture"));
	req.addChild(Tree("picture"));

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::send_avatar(const std::string & avatar)
{
	Tree pic("picture"); pic.setData(avatar);
	Tree prev("picture", makeAttr1("type", "preview")); prev.setData(avatar);

	Tree req("iq", makeAttr4("id", "set_photo_"+int2str(iqid++), "type", "set", "to", phone + "@" + whatsappserver, "xmlns", "w:profile:picture"));
	req.addChild(pic);
	req.addChild(prev);

	outbuffer = outbuffer + serialize_tree(&req);
}

bool WhatsappConnection::queryReceivedMessage(char *msgid, int * type)
{
	if (received_messages.size() == 0) return false;

	strcpy(msgid, received_messages[0].first.c_str());
	*type = received_messages[0].second;
	received_messages.erase(received_messages.begin());

	return true;
}

void WhatsappConnection::getMessageId(char * msgid)
{
	unsigned int t = time(NULL);
	unsigned int mid = msgcounter++;

	sprintf(msgid, "%u-%u", t, mid);
}

void WhatsappConnection::sendChat(std::string msgid, std::string to, std::string message)
{
	ChatMessage msg(this, to, time(NULL), msgid, message, nickname);
	DataBuffer buf = msg.serialize();

	outbuffer = outbuffer + buf;
}

void WhatsappConnection::sendGroupChat(std::string msgid, std::string to, std::string message)
{
	ChatMessage msg(this, to, time(NULL), msgid, message, nickname);
	msg.server = "g.us";
	DataBuffer buf = msg.serialize();

	outbuffer = outbuffer + buf;
}

void WhatsappConnection::addContacts(std::vector < std::string > clist)
{
	/* Insert the contacts to the contact list */
	for (unsigned int i = 0; i < clist.size(); i++) {
		if (contacts.find(clist[i]) == contacts.end())
			contacts[clist[i]] = Contact(clist[i], true);
		else
			contacts[clist[i]].mycontact = true;

		user_changes.push_back(clist[i]);
	}
	/* Query the profile pictures */
	bool qstatus = false;
	for (std::map < std::string, Contact >::iterator iter = contacts.begin(); iter != contacts.end(); iter++) {
		if (not iter->second.subscribed) {
			iter->second.subscribed = true;

			this->subscribePresence(iter->first + "@" + whatsappserver);
			this->queryPreview(iter->first + "@" + whatsappserver);
			this->getLast(iter->first + "@" + whatsappserver);
			qstatus = true;
		}
	}
	/* Query statuses */
	if (qstatus)
		this->queryStatuses();
}

unsigned char hexchars(char c1, char c2)
{
	if (c1 >= '0' and c1 <= '9')
		c1 -= '0';
	else if (c1 >= 'A' and c1 <= 'F')
		c1 = c1 - 'A' + 10;
	else if (c1 >= 'a' and c1 <= 'f')
		c1 = c1 - 'a' + 10;

	if (c2 >= '0' and c2 <= '9')
		c2 -= '0';
	else if (c2 >= 'A' and c2 <= 'F')
		c2 = c2 - 'A' + 10;
	else if (c2 >= 'a' and c2 <= 'f')
		c2 = c2 - 'a' + 10;

	unsigned char r = c2 | (c1 << 4);
	return r;
}

std::string UnicodeToUTF8(unsigned int c)
{
	std::string ret;
	if (c <= 0x7F)
		ret += ((char)c);
	else if (c <= 0x7FF) {
		ret += ((char)(0xC0 | (c >> 6)));
		ret += ((char)(0x80 | (c & 0x3F)));
	} else if (c <= 0xFFFF) {
		if (c >= 0xD800 and c <= 0xDFFF)
			return ret;	/* Invalid char */
		ret += ((char)(0xE0 | (c >> 12)));
		ret += ((char)(0x80 | ((c >> 6) & 0x3F)));
		ret += ((char)(0x80 | (c & 0x3F)));
	}
	return ret;
}

std::string utf8_decode(std::string in)
{
	std::string dec;
	for (unsigned int i = 0; i < in.size(); i++) {
		if (in[i] == '\\' and in[i + 1] == 'u') {
			i += 2;	/* Skip \u */
			unsigned char hex1 = hexchars(in[i + 0], in[i + 1]);
			unsigned char hex2 = hexchars(in[i + 2], in[i + 3]);
			unsigned int uchar = (hex1 << 8) | hex2;
			dec += UnicodeToUTF8(uchar);
			i += 3;
		} else if (in[i] == '\\' and in[i + 1] == '"') {
			dec += '"';
			i++;
		} else
			dec += in[i];
	}
	return dec;
}

std::string query_field(std::string work, std::string lo, bool integer = false)
{
	size_t p = work.find("\"" + lo + "\"");
	if (p == std::string::npos)
		return "";

	work = work.substr(p + ("\"" + lo + "\"").size());

	p = work.find("\"");
	if (integer)
		p = work.find(":");
	if (p == std::string::npos)
		return "";

	work = work.substr(p + 1);

	p = 0;
	while (p < work.size()) {
		if (work[p] == '"' and(p == 0 or work[p - 1] != '\\'))
			break;
		p++;
	}
	if (integer) {
		p = 0;
		while (p < work.size()and work[p] >= '0' and work[p] <= '9')
			p++;
	}
	if (p == std::string::npos)
		return "";

	work = work.substr(0, p);

	return work;
}

void WhatsappConnection::updateContactStatuses(std::string json)
{
	while (true) {
		size_t offset = json.find("{");
		if (offset == std::string::npos)
			break;
		json = json.substr(offset + 1);

		/* Look for closure */
		size_t cl = json.find("{");
		if (cl == std::string::npos)
			cl = json.size();
		std::string work = json.substr(0, cl);

		/* Look for "n", the number and "w","t","s" */
		std::string n = query_field(work, "n");
		std::string w = query_field(work, "w", true);
		std::string t = query_field(work, "t", true);
		std::string s = query_field(work, "s");

		if (w == "1") {
			contacts[n].status = utf8_decode(s);
			contacts[n].last_status = str2lng(t);
		}

		json = json.substr(cl);
	}
}

void WhatsappConnection::updateFileUpload(std::string json)
{
	size_t offset = json.find("{");
	if (offset == std::string::npos)
		return;
	json = json.substr(offset + 1);

	/* Look for closure */
	size_t cl = json.find("{");
	if (cl == std::string::npos)
		cl = json.size();
	std::string work = json.substr(0, cl);

	std::string url = query_field(work, "url");
	std::string type = query_field(work, "type");
	std::string size = query_field(work, "size");
	std::string width = query_field(work, "width");
	std::string height = query_field(work, "height");
	std::string filehash = query_field(work, "filehash");
	std::string mimetype = query_field(work, "mimetype");

	std::string to;
	for (unsigned int j = 0; j < uploadfile_queue.size(); j++)
		if (uploadfile_queue[j].uploading and uploadfile_queue[j].hash == filehash) {
			to = uploadfile_queue[j].to;
			uploadfile_queue.erase(uploadfile_queue.begin() + j);
			break;
		}
	/* Send the message with the URL :) */
	ImageMessage msg(this, to, time(NULL), int2str(msgcounter++), "author", url, str2int(width), str2int(height), str2int(size), "encoding", filehash, mimetype, temp_thumbnail);

	DataBuffer buf = msg.serialize();

	outbuffer = outbuffer + buf;
}

/* Quick and dirty way to parse the HTTP responses */
void WhatsappConnection::processSSLIncomingData()
{
	/* Parse HTTPS headers and JSON body */
	if (sslstatus == 1)
		sslstatus++;

	if (sslstatus == 2) {
		/* Look for the first line, to be 200 OK */
		std::string toparse((char *)sslbuffer_in.getPtr(), sslbuffer_in.size());
		if (toparse.find("\r\n") != std::string::npos) {
			std::string fl = toparse.substr(0, toparse.find("\r\n"));
			if (fl.find("200") == std::string::npos)
				goto abortStatus;

			if (toparse.find("\r\n\r\n") != std::string::npos) {
				std::string headers = toparse.substr(0, toparse.find("\r\n\r\n") + 4);
				std::string content = toparse.substr(toparse.find("\r\n\r\n") + 4);

				/* Look for content length */
				if (headers.find("Content-Length:") != std::string::npos) {
					std::string clen = headers.substr(headers.find("Content-Length:") + strlen("Content-Length:"));
					clen = clen.substr(0, clen.find("\r\n"));
					while (clen.size() > 0 and clen[0] == ' ')
						clen = clen.substr(1);
					unsigned int contentlength = str2int(clen);
					if (contentlength == content.size()) {
						/* Now we can proceed to parse the JSON */
						updateFileUpload(content);
						sslstatus = 0;
					}
				}
			}
		}
	}

	processUploadQueue();
	return;
abortStatus:
	sslstatus = 0;
	processUploadQueue();
	return;
}

std::string WhatsappConnection::generateUploadPOST(t_fileupload * fu)
{
	std::string file_buffer;
	FILE *fd = fopen(fu->file.c_str(), "rb");
	int read = 0;
	do {
		char buf[1024];
		read = fread(buf, 1, 1024, fd);
		file_buffer += std::string(buf, read);
	} while (read > 0);
	fclose(fd);

	std::string mime_type = std::string(file_mime_type(fu->file.c_str(), file_buffer.c_str(), file_buffer.size()));
	std::string encoded_name = "TODO..:";

	std::string ret;
	/* BODY HEAD */
	ret += "--zzXXzzYYzzXXzzQQ\r\n";
	ret += "Content-Disposition: form-data; name=\"to\"\r\n\r\n";
	ret += fu->to + "\r\n";
	ret += "--zzXXzzYYzzXXzzQQ\r\n";
	ret += "Content-Disposition: form-data; name=\"from\"\r\n\r\n";
	ret += fu->from + "\r\n";
	ret += "--zzXXzzYYzzXXzzQQ\r\n";
	ret += "Content-Disposition: form-data; name=\"file\"; filename=\"" + encoded_name + "\"\r\n";
	ret += "Content-Type: " + mime_type + "\r\n\r\n";

	/* File itself */
	ret += file_buffer;

	/* TAIL */
	ret += "\r\n--zzXXzzYYzzXXzzQQ--\r\n";

	std::string post;
	post += "POST " + fu->uploadurl + "\r\n";
	post += "Content-Type: multipart/form-data; boundary=zzXXzzYYzzXXzzQQ\r\n";
	post += "Host: " + fu->host + "\r\n";
	post += "User-Agent: WhatsApp/2.4.7 S40Version/14.26 Device/Nokia302\r\n";
	post += "Content-Length:  " + int2str(ret.size()) + "\r\n\r\n";

	std::string all = post + ret;

	fu->totalsize = file_buffer.size();

	return all;
}

void WhatsappConnection::processUploadQueue()
{
	/* At idle check for new uploads */
	if (sslstatus == 0) {
		for (unsigned int j = 0; j < uploadfile_queue.size(); j++) {
			if (uploadfile_queue[j].uploadurl != "" and not uploadfile_queue[j].uploading) {
				uploadfile_queue[j].uploading = true;
				std::string postq = generateUploadPOST(&uploadfile_queue[j]);

				sslbuffer_in.clear();
				sslbuffer.clear();

				sslbuffer.addData(postq.c_str(), postq.size());

				sslstatus = 1;
				break;
			}
		}
	}
}

void WhatsappConnection::processIncomingData()
{
	/* Parse the data and create as many Trees as possible */
	std::vector < Tree > treelist;
	if (inbuffer.size() >= 3) {
		/* Consume as many trees as possible */
		bool ok;
		do {
			Tree t;
			ok = parse_tree(&inbuffer, t);
			if (ok)
				treelist.push_back(t);
		} while (ok and inbuffer.size() >= 3);
	}

	/* Now process the tree list! */
	for (unsigned int i = 0; i < treelist.size(); i++) {
		DEBUG_PRINT( treelist[i].toString() );
		if (treelist[i].getTag() == "challenge") {
			/* Generate a session key using the challege & the password */
			assert(conn_status == SessionWaitingChallenge);

			KeyGenerator::generateKeysV14(password, treelist[i].getData().c_str(), treelist[i].getData().size(), (char *)this->session_key);

			in  = new RC4Decoder(&session_key[20*2], 20, 768);
			out = new RC4Decoder(&session_key[20*0], 20, 768);

			conn_status = SessionWaitingAuthOK;
			challenge_data = treelist[i].getData();

			this->sendResponse();
		} else if (treelist[i].getTag() == "success") {
			/* Notifies the success of the auth */
			conn_status = SessionConnected;
			if (treelist[i].hasAttribute("status"))
				this->account_status = treelist[i].getAttributes()["status"];
			if (treelist[i].hasAttribute("kind"))
				this->account_type = treelist[i].getAttributes()["kind"];
			if (treelist[i].hasAttribute("expiration"))
				this->account_expiration = treelist[i].getAttributes()["expiration"];
			if (treelist[i].hasAttribute("creation"))
				this->account_creation = treelist[i].getAttributes()["creation"];

			this->notifyMyPresence();
			this->sendInitial();  // Seems to trigger an error IQ response
			this->updateGroups();

			DEBUG_PRINT("Logged in!!!");
		} else if (treelist[i].getTag() == "failure") {
			if (conn_status == SessionWaitingAuthOK)
				this->notifyError(errorAuth);
			else
				this->notifyError(errorUnknown);
		} else if (treelist[i].getTag() == "notification") {
			DataBuffer reply = generateResponse(
				treelist[i].getAttribute("from"),
			    treelist[i].getAttribute("type"),
			    treelist[i].getAttribute("id")
		    );
			outbuffer = outbuffer + reply;
			
			if (treelist[i].hasAttributeValue("type", "participant") || 
				treelist[i].hasAttributeValue("type", "owner") ) {
				/* If the nofitication comes from a group, assume we have to reload groups ;) */
				updateGroups();
			}
		} else if (treelist[i].getTag() == "ack") {
			std::string id = treelist[i].getAttribute("id");
			received_messages.push_back( std::make_pair(id,0) );

		} else if (treelist[i].getTag() == "receipt") {
			std::string id = treelist[i].getAttribute("id");
			std::string type = treelist[i].getAttribute("type");
			if (type == "") type = "delivery";
			
			Tree mes("ack", makeAttr3("class", "receipt", "type", type, "id", id));
			outbuffer = outbuffer + serialize_tree(&mes);

			// Add reception package to queue
			int rtype = 1;
			if (type == "read") rtype = 2;
			received_messages.push_back( std::make_pair(id,rtype) );
			
		} else if (treelist[i].getTag() == "chatstate") {
			if (treelist[i].hasChild("composing"))
				this->gotTyping(treelist[i].getAttribute("from"), "composing");
			if (treelist[i].hasChild("paused"))
				this->gotTyping(treelist[i].getAttribute("from"), "paused");
			
		} else if (treelist[i].getTag() == "message") {
			/* Receives a message! */
			DEBUG_PRINT("Received message stanza...");
			if (treelist[i].hasAttribute("from") and
				(treelist[i].hasAttributeValue("type", "text") or treelist[i].hasAttributeValue("type", "media"))) {
				unsigned long long time = 0;
				if (treelist[i].hasAttribute("t"))
					time = str2lng(treelist[i].getAttribute("t"));
				std::string from = treelist[i].getAttribute("from");
				std::string id = treelist[i].getAttribute("id");
				std::string author = treelist[i].getAttribute("participant");

				Tree t;
				if (treelist[i].getChild("body", t)) {
					this->receiveMessage(ChatMessage(this, from, time, id, t.getData(), author));
				}
				if (treelist[i].getChild("media", t)) {
					if (t.hasAttributeValue("type", "image")) {
						this->receiveMessage(ImageMessage(this, from, time, id, author, t.getAttribute("url"), str2int(t.getAttribute("width")), str2int(t.getAttribute("height")), str2int(t.getAttribute("size")), t.getAttribute("encoding"), t.getAttribute("filehash"), t.getAttribute("mimetype"), t.getData()));
					} else if (t.hasAttributeValue("type", "location")) {
						this->receiveMessage(LocationMessage(this, from, time, id, author, str2dbl(t.getAttribute("latitude")), str2dbl(t.getAttribute("longitude")), t.getData()));
					} else if (t.hasAttributeValue("type", "audio")) {
						this->receiveMessage(SoundMessage(this, from, time, id, author, t.getAttribute("url"), t.getAttribute("filehash"), t.getAttribute("mimetype")));
					} else if (t.hasAttributeValue("type", "video")) {
						this->receiveMessage(VideoMessage(this, from, time, id, author, t.getAttribute("url"), t.getAttribute("filehash"), t.getAttribute("mimetype")));
					}
				}
			} else if (treelist[i].hasAttributeValue("type", "notification") and treelist[i].hasAttribute("from")) {
				/* If the nofitication comes from a group, assume we have to reload groups ;) */
				updateGroups();
			}
			/* Generate response for the messages */
			if (treelist[i].hasAttribute("type") and treelist[i].hasAttribute("from")) { //FIXME
				DataBuffer reply = generateResponse(treelist[i].getAttribute("from"),
								    "",
								    treelist[i].getAttribute("id")
								    );
				outbuffer = outbuffer + reply;
			}
		} else if (treelist[i].getTag() == "presence") {
			/* Receives the presence of the user, for v14 type is optional */
			if (treelist[i].hasAttribute("from")) {
				this->notifyPresence(treelist[i].getAttribute("from"), treelist[i].getAttribute("type"));
			}
		} else if (treelist[i].getTag() == "iq") {
			/* Receives the presence of the user */
			if (atoi(treelist[i].getAttribute("id").c_str()) == gw1)
				gq_stat |= 2;
			if (atoi(treelist[i].getAttribute("id").c_str()) == gw2)
				gq_stat |= 4;

			if (treelist[i].hasAttributeValue("type", "result") and treelist[i].hasAttribute("from")) {
				Tree t;
				if (treelist[i].getChild("query", t)) {
					if (t.hasAttribute("seconds")) {
						this->notifyLastSeen(treelist[i].getAttribute("from"), t.getAttribute("seconds"));
					}
				}
				if (treelist[i].getChild("picture", t)) {
					if (t.hasAttributeValue("type", "preview"))
						this->addPreviewPicture(treelist[i].getAttribute("from"), t.getData());
					if (t.hasAttributeValue("type", "image"))
						this->addFullsizePicture(treelist[i].getAttribute("from"), t.getData());
				}
				if (treelist[i].getChild("media", t)) {
					for (unsigned int j = 0; j < uploadfile_queue.size(); j++) {
						if (uploadfile_queue[j].rid == str2int(treelist[i].getAttribute("id"))) {
							/* Queue to upload the file */
							uploadfile_queue[j].uploadurl = t.getAttribute("url");
							std::string host = uploadfile_queue[j].uploadurl.substr(8);	/* Remove https:// */
							for (unsigned int i = 0; i < host.size(); i++)
								if (host[i] == '/')
									host = host.substr(0, i);
							uploadfile_queue[j].host = host;

							this->processUploadQueue();
							break;
						}
					}
				}

				if (treelist[i].getChild("duplicate", t)) {
					for (unsigned int j = 0; j < uploadfile_queue.size(); j++) {
						if (uploadfile_queue[j].rid == str2int(treelist[i].getAttribute("id"))) {
							/* Generate a fake JSON and process directly */
							std::string json = "{\"name\":\"" + uploadfile_queue[j].file + "\"," "\"url\":\"" + t.getAttribute("url") + "\"," "\"size\":\"" + t.getAttribute("size") + "\"," "\"mimetype\":\"" + t.getAttribute("mimetype") + "\"," "\"filehash\":\"" + t.getAttribute("filehash") + "\"," "\"type\":\"" + t.getAttribute("type") + "\"," "\"width\":\"" + t.getAttribute("width") + "\"," "\"height\":\"" + t.getAttribute("height") + "\"}";

							uploadfile_queue[j].uploading = true;
							this->updateFileUpload(json);
							break;
						}
					}
				}

				// Status result
				if (treelist[i].getChild("status", t)) {
					std::vector < Tree > childs = t.getChildren();
					for (unsigned int j = 0; j < childs.size(); j++) {
						if (childs[j].getTag() == "user") {
							std::string user = getusername(childs[j].getAttribute("jid"));
							contacts[user].status = utf8_decode(childs[j].getData());
						}
					}
				}

				std::vector < Tree > childs = treelist[i].getChildren();
				int acc = 0;
				for (unsigned int j = 0; j < childs.size(); j++) {
					if (childs[j].getTag() == "group") {
						bool rep = groups.find(getusername(childs[j].getAttribute("id"))) != groups.end();
						if (not rep) {
							groups.insert(std::pair < std::string, Group > (getusername(childs[j].getAttribute("id")), Group(getusername(childs[j].getAttribute("id")), childs[j].getAttribute("subject"), getusername(childs[j].getAttribute("owner")))));

							/* Query group participants */
							Tree iq("list");
							Tree req("iq", makeAttr4("id", int2str(iqid++), "type", "get", 
									"to", childs[j].getAttribute("id") + "@g.us", "xmlns", "w:g"));
							req.addChild(iq);
							gw3++;
							outbuffer = outbuffer + serialize_tree(&req);
						}
					} else if (childs[j].getTag() == "participant") {
						std::string gid = getusername(treelist[i].getAttribute("from"));
						std::string pt = getusername(childs[j].getAttribute("jid"));
						if (groups.find(gid) != groups.end()) {
							groups.find(gid)->second.participants.push_back(pt);
						}
						if (!acc)
							gw3--;
						acc = 1;
					} else if (childs[j].getTag() == "add") {

					}
				}

				if (treelist[i].getChild("group", t)) {
					if (t.hasAttributeValue("type", "preview"))
						this->addPreviewPicture(treelist[i].getAttribute("from"), t.getData());
					if (t.hasAttributeValue("type", "image"))
						this->addFullsizePicture(treelist[i].getAttribute("from"), t.getData());
				}
			}
			if (treelist[i].hasAttribute("from") and treelist[i].hasAttribute("id") and 
				treelist[i].hasAttributeValue("xmlns","urn:xmpp:ping")) {
				this->doPong(treelist[i].getAttribute("id"), treelist[i].getAttribute("from"));
			}
		}
	}

	if (gq_stat == 8 and recv_messages_delay.size() != 0) {
		DEBUG_PRINT ("Delayed messages -> Messages");
		for (unsigned int i = 0; i < recv_messages_delay.size(); i++) {
			recv_messages.push_back(recv_messages_delay[i]);
		}
		recv_messages_delay.clear();
	}
}

DataBuffer WhatsappConnection::serialize_tree(Tree * tree, bool crypt)
{
	DEBUG_PRINT( tree->toString() );

	DataBuffer data = write_tree(tree);
	if (data.size() > 65535) {
		std::cerr << "Skipping huge tree! " << data.size() << std::endl;
		return DataBuffer();
	}
	unsigned char flag = 0;
	if (crypt) {
		data = data.encodedBuffer(this->out, &this->session_key[20*1], true, this->frame_seq++);
		flag = 0x80;
	}

	DataBuffer ret;
	ret.putInt(flag, 1);
	ret.putInt(data.size(), 2);
	ret = ret + data;
	return ret;
}

DataBuffer WhatsappConnection::write_tree(Tree * tree)
{
	DataBuffer bout;
	int len = 1;

	if (tree->getAttributes().size() != 0)
		len += tree->getAttributes().size() * 2;
	if (tree->getChildren().size() != 0)
		len++;
	if (tree->getData().size() != 0 or tree->forcedData())
		len++;

	bout.writeListSize(len);
	if (tree->getTag() == "start")
		bout.putInt(1, 1);
	else
		bout.putString(tree->getTag());
	tree->writeAttributes(&bout);

	if (tree->getData().size() > 0 or tree->forcedData())
		bout.putRawString(tree->getData());
	if (tree->getChildren().size() > 0) {
		bout.writeListSize(tree->getChildren().size());

		for (unsigned int i = 0; i < tree->getChildren().size(); i++) {
			DataBuffer tt = write_tree(&tree->getChildren()[i]);
			bout = bout + tt;
		}
	}
	return bout;
}

bool WhatsappConnection::parse_tree(DataBuffer * data, Tree & t)
{
	int bflag = (data->getInt(1) & 0xF0) >> 4;
	int bsize = data->getInt(2, 1);
	if (bsize > data->size() - 3) {
		return false; /* Next message incomplete, return consumed data */
	}
	data->popData(3);

	if (bflag & 8) {
		/* Decode data, buffer conversion */
		if (this->in != NULL) {
			DataBuffer *decoded_data = data->decodedBuffer(this->in, bsize, false);

			bool res = read_tree(decoded_data, t);

			/* Call recursive */
			data->popData(bsize);	/* Pop data unencrypted for next parsing! */
			
			/* Remove hash */
			decoded_data->popData(4); 
			
			delete decoded_data;
			
			return res;
		} else {
			printf("Received crypted data before establishing crypted layer! Skipping!\n");
			data->popData(bsize);
			return false;
		}
	} else {
		return read_tree(data, t);
	}
}

bool WhatsappConnection::read_tree(DataBuffer * data, Tree & tt)
{
	int lsize = data->readListSize();
	int type = data->getInt(1);
	if (type == 1) {
		data->popData(1);
		Tree t;
		t.readAttributes(data, lsize);
		t.setTag("start");
		tt = t;
		return true;
	} else if (type == 2) {
		data->popData(1);
		return false;
	}

	Tree t;
	t.setTag(data->readString());
	t.readAttributes(data, lsize);

	if ((lsize & 1) == 1) {
		tt = t;
		return true;
	}

	if (data->isList()) {
		t.setChildren(data->readList(this));
	} else {
		t.setData(data->readString());
	}

	tt = t;
	return true;
}

static int isgroup(const std::string user)
{
	return (user.find('-') != std::string::npos);
}

void WhatsappConnection::receiveMessage(const Message & m)
{
	/* Push message to user and generate a response */
	Message *mc = m.copy();
	
	if (isgroup(m.from) and gq_stat != 8)	{/* Delay the group message deliver if we do not have the group list */
		recv_messages_delay.push_back(mc);
		DEBUG_PRINT("Received delayed message!");
	}else
		recv_messages.push_back(mc);

	DEBUG_PRINT("Received message type " << m.type() << " from " << m.from << " at " << m.t);

	/* Now add the contact in the list (to query the profile picture) */
	if (contacts.find(m.from) == contacts.end())
		contacts[m.from] = Contact(m.from, false);
	this->addContacts(std::vector < std::string > ());
}

void WhatsappConnection::notifyLastSeen(std::string from, std::string seconds)
{
	from = getusername(from);
	contacts[from].last_seen = str2lng(seconds);
}

void WhatsappConnection::notifyPresence(std::string from, std::string status)
{
	if (status == "")
		status = "available";
	from = getusername(from);
	contacts[from].presence = status;
	user_changes.push_back(from);
}

void WhatsappConnection::addPreviewPicture(std::string from, std::string picture)
{
	from = getusername(from);
	if (contacts.find(from) == contacts.end()) {
		Contact newc(from, false);
		contacts[from] = newc;
	}
	contacts[from].ppprev = picture;
	user_icons.push_back(from);
}

void WhatsappConnection::addFullsizePicture(std::string from, std::string picture)
{
	from = getusername(from);
	if (contacts.find(from) == contacts.end()) {
		Contact newc(from, false);
		contacts[from] = newc;
	}
	contacts[from].pppicture = picture;
}

void WhatsappConnection::setMyPresence(std::string s, std::string msg)
{
	sendRead = (s == "available");
	if (s == "available-noread") {
		s = "available";
	}

	if (s != mypresence) {
		mypresence = s;
		notifyMyPresence();
	}
	if (msg != mymessage) {
		mymessage = msg;
		notifyMyMessage();	/*TODO */
	}
}

void WhatsappConnection::notifyMyPresence()
{
	/* Send the nickname and the current status */
	Tree pres("presence", makeAttr2("name", nickname, "type", mypresence));

	outbuffer = outbuffer + serialize_tree(&pres);
}

void WhatsappConnection::sendInitial()
{
	Tree conf("config");
	Tree iq("iq", makeAttr4("id", int2str(iqid++), "type", "get", "to", whatsappserver, "xmlns", "urn:xmpp:whatsapp:push"));
	iq.addChild(conf);	

	outbuffer = outbuffer + serialize_tree(&iq);
}

void WhatsappConnection::notifyMyMessage()
{
	/* Send the status message */
	Tree xhash("x", makeAttr1("xmlns", "jabber:x:event"));
	xhash.addChild(Tree("server"));
	Tree tbody("body");
	tbody.setData(this->mymessage);

	Tree mes("message", makeAttr3("to", "s.us", "type", "chat", "id", int2str(time(NULL)) + "-" + int2str(iqid++)));
	mes.addChild(xhash);
	mes.addChild(tbody);

	outbuffer = outbuffer + serialize_tree(&mes);
}

void WhatsappConnection::notifyError(ErrorCode err)
{

}

// Returns an integer indicating the next message type (sorting by timestamp)
int WhatsappConnection::query_next() {
	int res = -1;
	unsigned int cur_ts = ~0;
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->t < cur_ts) {
			cur_ts = recv_messages[i]->t;
			res = recv_messages[i]->type();
		}
	}
	return res;
}

bool WhatsappConnection::query_chat(std::string & from, std::string & message, std::string & author, unsigned long &t)
{
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 0) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			message = ((ChatMessage *) recv_messages[i])->message;
			author = ((ChatMessage *) recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin() + i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatimages(std::string & from, std::string & preview, std::string & url, std::string & author, unsigned long &t)
{
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 1) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			preview = ((ImageMessage *) recv_messages[i])->preview;
			url = ((ImageMessage *) recv_messages[i])->url;
			author = ((ImageMessage *) recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin() + i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatsounds(std::string & from, std::string & url, std::string & author, unsigned long &t)
{
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 3) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			url = ((SoundMessage *) recv_messages[i])->url;
			author = ((SoundMessage *) recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin() + i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatvideos(std::string & from, std::string & url, std::string & author, unsigned long &t)
{
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 4) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			url = ((VideoMessage *) recv_messages[i])->url;
			author = ((VideoMessage *) recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin() + i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatlocations(std::string & from, double &lat, double &lng, std::string & prev, std::string & author, unsigned long &t)
{
	for (unsigned int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 2) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			prev = ((LocationMessage *) recv_messages[i])->preview;
			lat = ((LocationMessage *) recv_messages[i])->latitude;
			lng = ((LocationMessage *) recv_messages[i])->longitude;
			author = ((LocationMessage *) recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin() + i);
			return true;
		}
	}
	return false;
}

int WhatsappConnection::getuserstatus(const std::string & who)
{
	if (contacts.find(who) != contacts.end()) {
		if (contacts[who].presence == "available")
			return 1;
		return 0;
	}
	return -1;
}

std::string WhatsappConnection::getuserstatusstring(const std::string & who)
{
	if (contacts.find(who) != contacts.end()) {
		return contacts[who].status;
	}
	return "";
}

unsigned long long WhatsappConnection::getlastseen(const std::string & who)
{
	/* Schedule a last seen update, just in case */
	this->getLast(std::string(who) + "@" + whatsappserver);

	if (contacts.find(who) != contacts.end()) {
		return contacts[who].last_seen;
	}
	return ~0;
}

bool WhatsappConnection::query_status(std::string & from, int &status)
{
	while (user_changes.size() > 0) {
		if (contacts.find(user_changes[0]) != contacts.end()) {
			from = user_changes[0];
			status = 0;
			if (contacts[from].presence == "available")
				status = 1;

			user_changes.erase(user_changes.begin());
			return true;
		}
		user_changes.erase(user_changes.begin());
	}
	return false;
}

bool WhatsappConnection::query_typing(std::string & from, int &status)
{
	while (user_typing.size() > 0) {
		if (contacts.find(user_typing[0]) != contacts.end()) {
			from = user_typing[0];
			status = 0;
			if (contacts[from].typing == "composing")
				status = 1;

			user_typing.erase(user_typing.begin());
			return true;
		}
		user_typing.erase(user_typing.begin());
	}
	return false;
}

bool WhatsappConnection::query_icon(std::string & from, std::string & icon, std::string & hash)
{
	while (user_icons.size() > 0) {
		if (contacts.find(user_icons[0]) != contacts.end()) {
			from = user_icons[0];
			icon = contacts[from].ppprev;
			hash = "";

			user_icons.erase(user_icons.begin());
			return true;
		}
		user_icons.erase(user_icons.begin());
	}
	return false;
}

bool WhatsappConnection::query_avatar(std::string user, std::string & icon)
{
	user = getusername(user);
	if (contacts.find(user) != contacts.end()) {
		icon = contacts[user].pppicture;
		if (icon.size() == 0) {
			/* Return preview icon and query the fullsize picture */
			/* for future displays to save bandwidth */
			this->queryFullSize(user + "@" + whatsappserver);
			icon = contacts[user].ppprev;
		}
		return true;
	}
	return false;
}

void WhatsappConnection::doPong(std::string id, std::string from)
{
	Tree t("iq", makeAttr3("to",from, "id",id, "type","result"));
	outbuffer = outbuffer + serialize_tree(&t);
}

void WhatsappConnection::sendResponse()
{
	Tree t("response");

	std::string response = phone + challenge_data + int2str(time(NULL));
	DataBuffer eresponse(response.c_str(), response.size());
	eresponse = eresponse.encodedBuffer(this->out, &this->session_key[20*1], false, this->frame_seq++);
	response = eresponse.toString();
	t.setData(response);

	outbuffer = outbuffer + serialize_tree(&t, false);
}


