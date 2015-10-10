
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
#include <algorithm>
#include <map>
#include <vector>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "wadict.h"
#include "rc4.h"
#include "keygen.h"
#include "databuffer.h"
#include "tree.h"
#include "contacts.h"
#include "message.h"
#include "wa_connection.h"
#include "wa_util.h"
#include "wa_constants.h"

#include "keyhelper.h"
#include "prekeywhispermessage.h"
#include "sessioncipher.h"
#include "whisperexception.h"
#include "sessioncipher.h"

static int isbroadcast(const std::string user)
{
	return (user.find("@broadcast") != std::string::npos);
}

DataBuffer WhatsappConnection::generateResponse(std::string from, std::string type, std::string id)
{
	if (type == "") { // Auto 
		if (sendRead) type = "read";
		else type = "delivery";
	}
	Tree mes("receipt", makeat({"to", from, "id", id, "type", type, "t", std::to_string(1)}));

	return serialize_tree(&mes);
}

std::string WhatsappConnection::tohex(uint64_t n) {
	std::string ret;
	const char *hext = "0123456789abcdef";
	uint64_t cnum = n;
	while (cnum > 0) {
		ret += hext[cnum&15];
		cnum >>= 4;
	}
	return ret;
}

std::string WhatsappConnection::getNextIqId() {
	return tohex(++iqid);
}

/* Send image transaction */
int WhatsappConnection::sendImage(std::string mid, std::string to, int w, int h, unsigned int size, const char *fp)
{
	/* Type can be: audio/image/video */
	std::string siqid = getNextIqId();
	std::string sha256b64hash = SHA256_file_b64(fp);
	Tree iq("media", makeat({"type", "image", "hash", sha256b64hash, "size", std::to_string(size)}));
	Tree req("iq", makeat({"id", siqid, "type", "set", "to", whatsappserver, "xmlns", "w:m"}));
	req.addChild(iq);

	t_fileupload fu;
	fu.to = to;
	fu.file = std::string(fp);
	fu.rid = iqid;
	fu.hash = sha256b64hash;
	fu.type = "image";
	fu.uploading = false;
	fu.totalsize = 0;
	fu.thumbnail = getpreview(fp);
	fu.msgid = mid;
	uploadfile_queue.push_back(fu);
	outbuffer = outbuffer + serialize_tree(&req);

	return iqid;
}

WhatsappConnection::WhatsappConnection(std::string phonenum, std::string password, std::string nickname, std::string axolotldb)
{
	this->phone = phonenum;
	this->password = password;
	this->in = NULL;
	this->out = NULL;
	this->conn_status = SessionNone;
	this->msgcounter = 1;
	this->iqid = 0;
	this->nickname = nickname;
	this->whatsappserver = WHATSAPP_SERVER;
	this->whatsappservergroup = "g.us";
	this->mypresence = "available";
	this->groups_updated = false;
	this->blists_updated = false;
	this->sslstatus = 0;
	this->frame_seq = 0;
	this->sendRead = true;
	this->last_keepalive = 0;

	// Create in memory temp database!
	//if (axolotldb.size())
		this->axolotlStore.reset(new LiteAxolotlStore(axolotldb));
	//else
		//this->axolotlStore.reset(new InMemoryAxolotlStore());

	// Create in memory temp database!
	//if (axolotldb.size())
		this->axolotlStore.reset(new LiteAxolotlStore(axolotldb));
	//else
		//this->axolotlStore.reset(new InMemoryAxolotlStore());

	/* Trim password spaces */
	while (password.size() > 0 and password[0] == ' ')
		password = password.substr(1);
	while (password.size() > 0 and password[password.size() - 1] == ' ')
		password = password.substr(0, password.size() - 1);

	/* Remove non-numbers from phone */
	phone.erase(std::remove_if(phone.begin(), phone.end(), [](char ch){return !isdigit(ch);}), phone.end());
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

std::string WhatsappConnection::saveAxolotlDatabase()
{
	// Serialize the database
	//return axolotlStore->serialize();
	return "";
}

std::map < std::string, Group > WhatsappConnection::getGroups()
{
	return groups;
}

bool WhatsappConnection::groupsUpdated()
{
	bool r = groups_updated;
	groups_updated = false;
	return r;
}

void WhatsappConnection::updateGroups()
{
	/* Get the group list */
	groups.clear();
	{
		Tree req("iq", makeat({"id", getNextIqId(), "type", "get", "to", "g.us", "xmlns", "w:g2"}));
		req.addChild(Tree("participating"));
		outbuffer = outbuffer + serialize_tree(&req);
	}
}

void WhatsappConnection::manageParticipant(std::string group, std::string participant, std::string command)
{
	Tree iq(command);
	iq.addChild(Tree("participant", makeat({"jid", participant})));
	Tree req("iq", makeat({"id", getNextIqId(), "type", "set", "to", group + "@g.us", "xmlns", "w:g2"}));
	req.addChild(iq);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::leaveGroup(std::string group)
{
	Tree iq("leave");
	iq.addChild(Tree("group", makeat({"id", group + "@g.us"})));
	Tree req("iq", makeat({"id", getNextIqId(), "type", "set", "to", "g.us", "xmlns", "w:g2"}));
	req.addChild(iq);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::addGroup(std::string subject)
{
	Tree req("iq", makeat({"id", getNextIqId(), "type", "set", "to", "g.us", "xmlns", "w:g2"}));
	Tree create("create", makeat({"subject", subject}));
	req.addChild(create);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::updateBlists()
{
	blists.clear();
	Tree req("iq", makeat({
		"id", getNextIqId(),
		"from", phone + "@" + whatsappserver,
		"type", "get",
		"to", WHATSAPP_SERVER,
		"xmlns", "w:b"}
	));
	req.addChild(Tree("lists"));

	outbuffer = outbuffer + serialize_tree(&req);
}

bool WhatsappConnection::blistsUpdated()
{
	bool r = blists_updated;
	blists_updated = false;
	return r;
}

void WhatsappConnection::deleteBlist(std::string id)
{
	Tree req("iq", makeat({
		"id", getNextIqId(),
		"type", "set",
		"to", WHATSAPP_SERVER,
		"xmlns", "w:b"}
	));
	Tree del;
	del.addChild(Tree("list", makeat({"id", id + "@broadcast"})));
	req.addChild(del);

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::doLogin(std::string resource)
{
	/* Send stream init */
	DataBuffer first;
	error_queue.clear();

	{
		first.addData("WA\1\5", 4);
		Tree t("start", makeat({"resource",resource, "to",whatsappserver}));
		first = first + serialize_tree(&t, false);
	}

	/* Send features */
	{
		Tree p("stream:features");
		p.addChild(Tree("readreceipts"));
		p.addChild(Tree("privacy"));
		p.addChild(Tree("presence"));
		p.addChild(Tree("groups_v2"));
		first = first + serialize_tree(&p, false);
	}

	/* Send auth request */
	{
		Tree t("auth", makeat({"mechanism","WAUTH-2", "user",phone}));
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
	// Check whether we need to send a keepalive
	if (time(0) - last_keepalive > 30) {
		last_keepalive = time(0);
		if (conn_status == SessionConnected)
			notifyMyPresence();
	}

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

bool WhatsappConnection::hasSSLConnection(std::string & host, int & port)
{
	host = "";
	port = 443;

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
	Tree request("presence", makeat({"type", "subscribe", "to", user}));
	outbuffer = outbuffer + serialize_tree(&request);
}

void WhatsappConnection::queryStatuses()
{
	Tree req("iq", makeat({"to", WHATSAPP_SERVER, "type", "get", "id", getNextIqId(), "xmlns", "status"}));
	Tree stat("status");

	for (std::map < std::string, Contact >::iterator iter = contacts.begin(); iter != contacts.end(); iter++)
	{
		stat.addChild(Tree("user", makeat({"jid", iter->first + "@" + whatsappserver})));
	}
	req.addChild(stat);
	
	outbuffer = outbuffer + serialize_tree(&req);
}

std::string WhatsappConnection::syncContacts(std::vector < std::string > clist)
{
	std::string uid = getNextIqId();
	Tree req("iq", makeat({"id", uid, "type", "get", "xmlns", "urn:xmpp:whatsapp:sync"}));
	Tree sync("sync", makeat({"sid", std::to_string(time(0)), "index", "0", "mode", "full", "context", "registration", "last", "true"}));
	for (auto & u: clist) {
		Tree t("user");
		t.setData(u);
		sync.addChild(t);
	}
	req.addChild(sync);

	outbuffer = outbuffer + serialize_tree(&req);
	return uid;
}

bool WhatsappConnection::getSyncResult(std::string uid, std::vector<std::string> & out)
{
	if (sync_result.find(uid) == sync_result.end())
		return false;
	out = sync_result[uid];
	sync_result.erase(uid);
	return true;
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

	Tree mes("chatstate", makeat({"to", who + "@" + whatsappserver}));
	mes.addChild(Tree(s));

	outbuffer = outbuffer + serialize_tree(&mes);
}

void WhatsappConnection::account_info(unsigned long long &creation, unsigned long long &freeexp, std::string & status)
{
	creation = std::stoull(account_creation);
	freeexp = std::stoull(account_expiration);
	status = account_status;
}

void WhatsappConnection::queryPreview(std::string user)
{
	Tree req("iq", makeat({"id", getNextIqId(), "type", "get", "to", user, "xmlns", "w:profile:picture"}));
	req.addChild(Tree("picture", makeat({"type", "preview"})));

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::queryFullSize(std::string user)
{
	Tree req("iq", makeat({"id", getNextIqId(), "type", "get", "to", user, "xmlns", "w:profile:picture"}));
	req.addChild(Tree("picture"));

	outbuffer = outbuffer + serialize_tree(&req);
}

void WhatsappConnection::send_avatar(const std::string & avatar, const std::string & avatarp)
{
	Tree pic("picture"); pic.setData(avatar);
	Tree prev("picture", makeat({"type", "preview"})); prev.setData(avatarp);

	Tree req("iq", makeat({"id", "set_photo_"+getNextIqId(), "type", "set", "to", phone + "@" + whatsappserver, "xmlns", "w:profile:picture"}));
	req.addChild(pic);
	req.addChild(prev);

	outbuffer = outbuffer + serialize_tree(&req);
}

bool WhatsappConnection::queryReceivedMessage(std::string & msgid, int & type, unsigned long long & t, std::string & sender)
{
	if (received_messages.size() == 0) return false;

	msgid = received_messages[0].id;
	type = received_messages[0].type;
	t = received_messages[0].t;
	sender = received_messages[0].from;
	received_messages.erase(received_messages.begin());

	return true;
}

std::string WhatsappConnection::getMessageId()
{
	unsigned int t = time(NULL);
	unsigned int mid = msgcounter++;

	return std::to_string(t) + "-" + std::to_string(mid);
}

void WhatsappConnection::sendVCard(const std::string msgid, const std::string to, const std::string name, const std::string vcard)
{
	VCardMessage msg(this, to, time(NULL), msgid, nickname, name, vcard);
	DataBuffer buf = msg.serialize();

	outbuffer = outbuffer + buf;
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
}

void WhatsappConnection::contactsUpdate() {
	/* Query the profile pictures */
	bool qstatus = false;
	for (std::map < std::string, Contact >::iterator iter = contacts.begin(); iter != contacts.end(); iter++) {
		if (not iter->second.subscribed) {
			iter->second.subscribed = true;

			this->subscribePresence(iter->first + "@" + whatsappserver);
			this->queryPreview(iter->first + "@" + whatsappserver);
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
			contacts[n].last_status = std::stoull(t);
		}

		json = json.substr(cl);
	}
}

std::string base64_encode_esp(unsigned char const *bytes_to_encode, unsigned int in_len);

void WhatsappConnection::updateFileUpload(std::string json)
{
	DEBUG_PRINT("FILE UPLOAD:\n");
	DEBUG_PRINT(json);
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

	std::string to, thumb, ip, mid;
	for (unsigned int j = 0; j < uploadfile_queue.size(); j++)
		if (uploadfile_queue[j].uploading and uploadfile_queue[j].hash == filehash) {
			to = uploadfile_queue[j].to;
			thumb = uploadfile_queue[j].thumbnail;
			ip = uploadfile_queue[j].ip;
			mid = uploadfile_queue[j].msgid;
			uploadfile_queue.erase(uploadfile_queue.begin() + j);
			break;
		}
	/* Send the message with the URL :) */
	ImageMessage msg(this, to, time(NULL), mid, "author", url, "", ip, 
		std::stoi(width), std::stoi(height), std::stoi(size), "encoding", 
		filehash, mimetype, thumb);

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
					unsigned int contentlength = std::stoi(clen);
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
	post += WHATSAPP_USER_AGENT;
	post += "Content-Length:  " + std::to_string(ret.size()) + "\r\n\r\n";

	std::string all = post + ret;
	DEBUG_PRINT(post);

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
	//for (unsigned int i = 0; i < treelist.size(); i++) {
	for (auto & tl : treelist) {
		DEBUG_PRINT( tl.toString() );
		if (tl.getTag() == "challenge") {
			/* Generate a session key using the challege & the password */
			assert(conn_status == SessionWaitingChallenge);

			KeyGenerator::generateKeysV14(password, tl.getData().c_str(), tl.getData().size(), (char *)this->session_key);

			in  = new RC4Decoder(&session_key[20*2], 20, 768);
			out = new RC4Decoder(&session_key[20*0], 20, 768);

			conn_status = SessionWaitingAuthOK;
			challenge_data = tl.getData();

			this->sendResponse();
		} else if (tl.getTag() == "success") {
			/* Notifies the success of the auth */
			conn_status = SessionConnected;
			if (tl.hasAttribute("status"))
				this->account_status = tl["status"];
			if (tl.hasAttribute("kind"))
				this->account_type = tl["kind"];
			if (tl.hasAttribute("expiration"))
				this->account_expiration = tl["expiration"];
			if (tl.hasAttribute("creation"))
				this->account_creation = tl["creation"];

			this->notifyMyPresence();
			this->updatePrivacy();
			this->sendInitial();  // Seems to trigger an error IQ response
			this->updateGroups();
			this->updateBlists();

			/*resource.startsWith("S40") &&*/
			if (axolotlStore->countPreKeys() == 0)
				this->sendEncrypt(true);

			DEBUG_PRINT("Logged in!!!");
		} else if (tl.getTag() == "failure") {
			std::string reason = "unknown";
			if (tl.hasChild("not-authorized"))
				reason = "not-authorized";

			if (conn_status == SessionWaitingAuthOK)
				this->notifyError(errorAuth, reason);
			else
				this->notifyError(errorUnknown, reason);
		} else if (tl.getTag() == "notification") {
			DataBuffer reply = generateResponse( tl["from"], tl["type"], tl["id"] );
			outbuffer = outbuffer + reply;
			
			if (tl.hasAttributeValue("type", "participant") || 
				tl.hasAttributeValue("type", "owner") ||
				tl.hasAttributeValue("type", "w:gp2") ) {
				/* If the nofitication comes from a group, assume we have to reload groups ;) */
				updateGroups();
			}

			if (tl.hasAttributeValue("type", "picture")) {
				/* Picture update */
				this->queryPreview(tl["from"]);
			}
		} else if (tl.getTag() == "ack") {
			std::string id = tl["id"];
			unsigned long long t = 0;
			if (tl.hasAttribute("t"))
				t = std::stoull(tl["t"]);
			received_messages.push_back( {id, rSent, t, ""} );

		} else if (tl.getTag() == "receipt") {
			std::string id = tl["id"];
			std::string type = tl["type"];
			if (type == "") type = "delivery";
			std::string from = tl["from"];
			std::string to = tl["to"];
			std::string participant = tl["participant"];
			unsigned long long t = 0;
			if (tl.hasAttribute("t"))
				t = std::stoull(tl["t"]);
			
			Tree mes("ack", makeat({"class", "receipt", "type", type, "id", id}));

			// Add optional fields
			if (from != "") mes["to"] = from;
			if (to != "") mes["from"] = to;
			if (participant != "") mes["participant"] = participant;

			outbuffer = outbuffer + serialize_tree(&mes);

			// Add reception package to queue
			std::string who = getusername(participant.size() ? participant : from);
			ReceptionType rtype = rDelivered;
			if (type == "read") rtype = rRead;
			received_messages.push_back( {id,rtype,t, } );
			
		} else if (tl.getTag() == "chatstate") {
			if (tl.hasChild("composing"))
				this->gotTyping(tl["from"], "composing");
			if (tl.hasChild("paused"))
				this->gotTyping(tl["from"], "paused");
			
		} else if (tl.getTag() == "message") {
			/* Receives a message! */
			DEBUG_PRINT("Received message stanza...");
			bool donotreply = false;
			if (tl.hasAttribute("from") and
				(tl.hasAttributeValue("type", "text") or tl.hasAttributeValue("type", "media"))) {
				unsigned long long time = 0;
				if (tl.hasAttribute("t"))
					time = std::stoull(tl["t"]);
				std::string from = tl["from"];
				std::string id = tl["id"];
				std::string author = tl["participant"];

				if (isbroadcast(from))
					from = author;

				Tree t;
				if (tl.getChild("body", t)) {
					this->receiveMessage(ChatMessage(this, from, time, id, t.getData(), author));
				}
				if (tl.getChild("enc", t)) {
					if (!this->receiveCipheredMessage(from, id, author, time, t))
						donotreply = true;
					//this->receiveMessage(ChatMessage(this, from, time, id, "[Ciphered message received]", author));
					//DataBuffer reply = generateResponse(tl["from"], "retry", tl["id"]);
					//donotreply = true;
					//outbuffer = outbuffer + reply;
				}
				if (tl.getChild("media", t)) {
					if (t.hasAttributeValue("type", "image")) {
						this->receiveMessage(ImageMessage(this, from, time, id, author, 
							t["url"], t["caption"], t["ip"], std::stoi(t["width"]), std::stoi(t["height"]),
							std::stoi(t["size"]), t["encoding"], t["filehash"], t["mimetype"],
							t.getData()));
					} else if (t.hasAttributeValue("type", "location")) {
						this->receiveMessage(LocationMessage(this, from, time, id, author, str2dbl(t["latitude"]), str2dbl(t["longitude"]), t["name"], t.getData()));
					} else if (t.hasAttributeValue("type", "audio")) {
						this->receiveMessage(SoundMessage(this, from, time, id, author, t["url"], t["caption"], t["filehash"], t["mimetype"]));
					} else if (t.hasAttributeValue("type", "video")) {
						this->receiveMessage(VideoMessage(this, from, time, id, author, t["url"], t["caption"], t["filehash"], t["mimetype"]));
					} else if (t.hasAttributeValue("type", "vcard")) {
						Tree vc;
						if (t.getChild("vcard", vc))
							this->receiveMessage(VCardMessage(this, from, time, id, author, t["name"], vc.getData()));
					}
				}
			} else if (tl.hasAttributeValue("type", "notification") and tl.hasAttribute("from")) {
				/* If the nofitication comes from a group, assume we have to reload groups ;) */
				updateGroups();
			}
			/* Generate response for the messages */
			if (tl.hasAttribute("type") and tl.hasAttribute("from") and not donotreply) { //FIXME
				DataBuffer reply = generateResponse(tl["from"], "", tl["id"]);
				outbuffer = outbuffer + reply;
			}
		} else if (tl.getTag() == "call") {
			if (tl.hasAttribute("notify")) {
				unsigned long long time = 0;
				if (tl.hasAttribute("t"))
					time = std::stoull(tl["t"]);
				std::string from = tl["from"];
				std::string id = tl["id"];

				this->receiveMessage(CallMessage(this, from, time, id));
			}
			DataBuffer reply = generateResponse(tl["from"], "", tl["id"]);
			outbuffer = outbuffer + reply;
		} else if (tl.getTag() == "presence") {
			/* Receives the presence of the user, for v14 type is optional */
			if (tl.hasAttribute("from")) {
				this->notifyPresence(tl["from"], tl["type"], tl["last"]);
			}
		} else if (tl.getTag() == "iq") {
			/* Receives the presence of the user */
			if (tl.hasAttributeValue("type", "result") and tl.hasAttribute("from")) {
				Tree t;
				if (tl.getChild("picture", t)) {
					if (t.hasAttributeValue("type", "preview"))
						this->addPreviewPicture(tl["from"], t.getData());
					if (t.hasAttributeValue("type", "image"))
						this->addFullsizePicture(tl["from"], t.getData());
				}
				if (tl.getChild("media", t)) {
					for (unsigned int j = 0; j < uploadfile_queue.size(); j++) {
						if (tohex(uploadfile_queue[j].rid) == tl["id"]) {
							/* Queue to upload the file */
							uploadfile_queue[j].uploadurl = t["url"];
							uploadfile_queue[j].ip = t["ip"];
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

				if (tl.getChild("duplicate", t)) {
					for (unsigned int j = 0; j < uploadfile_queue.size(); j++) {
						if (tohex(uploadfile_queue[j].rid) == tl["id"]) {
							/* Generate a fake JSON and process directly */
							std::string json = "{\"name\":\"" + uploadfile_queue[j].file + "\"," "\"url\":\"" + t["url"] + "\"," "\"size\":\"" + t["size"] + "\"," "\"mimetype\":\"" + t["mimetype"] + "\"," "\"filehash\":\"" + t["filehash"] + "\"," "\"type\":\"" + t["type"] + "\"," "\"width\":\"" + t["width"] + "\"," "\"height\":\"" + t["height"] + "\"}";

							uploadfile_queue[j].uploading = true;
							this->updateFileUpload(json);
							break;
						}
					}
				}

				// Status result
				if (tl.getChild("status", t)) {
					std::vector < Tree > childs = t.getChildren();
					for (unsigned int j = 0; j < childs.size(); j++) {
						if (childs[j].getTag() == "user") {
							std::string user = getusername(childs[j]["jid"]);
							contacts[user].status = utf8_decode(childs[j].getData());
						}
					}
				}

				std::vector < Tree > childs = tl.getChildren();
				for (unsigned int j = 0; j < childs.size(); j++) {
					if (childs[j].getTag() == "groups") {
						std::vector < Tree > cgroups = childs[j].getChildren();
						for (auto & g : cgroups) {
							if (g.getTag() != "group") continue;
							bool rep = groups.find(getusername(g["id"])) != groups.end();
							if (not rep) {
								unsigned long long subjt = 0, creat = 0;
								if (g.hasAttribute("s_t"))
									subjt = std::stoull(g["s_t"]);
								if (g.hasAttribute("creation"))
									creat = std::stoull(g["creation"]);

								Group ng(
									getusername(g["id"]), g["subject"], subjt, 
									getusername(g["s_o"]),
									getusername(g["creator"]), creat
								);
								for (auto & pa: g.getChildren()) {
									if (pa.getTag() != "participant") continue;
									ng.participants.push_back(
										Group::Participant(getusername(pa["jid"]), pa["type"])
									);
								}

								groups.insert(	
									std::pair < std::string, Group > (
										getusername(g["id"]),
										ng
									)
								);
							}
						}
						groups_updated = true;
					} else if (childs[j].getTag() == "add") {
						//groups_updated = true;
					} else if (childs[j].getTag() == "lists") {
						// For every blist, add it to the blist vector
						std::vector < Tree > clists = childs[j].getChildren();
						for (unsigned int k = 0; k < clists.size(); k++) {
							if (clists[k].hasAttribute("id")) {
								BList bl(clists[k]["id"], clists[k]["name"]);
								std::vector < Tree > parts = clists[k].getChildren();
								for (unsigned int l = 0; l < parts.size(); l++) {
									if (parts[l].getTag() == "recipient" && parts[l]["jid"] != "")
										bl.dests.push_back(parts[l]["jid"]);
								}
							}
						}
						blists_updated = true;
					}
				}

				if (tl.getChild("group", t)) {
					if (t.hasAttributeValue("type", "preview"))
						this->addPreviewPicture(tl["from"], t.getData());
					if (t.hasAttributeValue("type", "image"))
						this->addFullsizePicture(tl["from"], t.getData());
				}

				if (tl.getChild("privacy", t)) {
					for (auto & ct : t.getChildren()) {
						if (ct.hasAttributeValue("name","last"))
							this->show_last_seen = ct["value"];
						if (ct.hasAttributeValue("name","status"))
							this->show_status_msg = ct["value"];
						if (ct.hasAttributeValue("name","profile"))
							this->show_profile_pic = ct["value"];
					}
				}

				if (tl.getChild("sync", t)) {
					std::vector <std::string> ct_in, ct_out, ct_invalid;
					for (auto & tt: t.getChildren()) {
						for (auto & user: tt.getChildren()) {
							if (user.getTag() != "user") continue;
							if (tt.getTag() == "in")
								ct_in.push_back(getusername(user["jid"]));
							if (tt.getTag() == "out")
								ct_out.push_back(getusername(user["jid"]));
							if (tt.getTag() == "invalid")
								ct_invalid.push_back(getusername(user["jid"]));
						}
					}
					sync_result[tl["id"]] = ct_in;
				}
			}
			if (tl.hasAttribute("from") and tl.hasAttribute("id") and 
				tl.hasAttributeValue("xmlns","urn:xmpp:ping")) {
				this->doPong(tl["id"], tl["from"]);
			}
		}
	}
}

bool WhatsappConnection::receiveCipheredMessage(std::string from, std::string id,
	std::string author, unsigned long long time, Tree enc) {

	if (enc["type"] == "pkmsg")
		return this->parsePreKeyWhisperMessage(from, id, author, time, enc);
	else
		return this->parseWhisperMessage(from, id, author, time, enc);
}

static uint64_t JidAsInt(const std::string & s) {
	return std::stoull(s.substr(0, s.find("@")));
}

static std::string numToBytesZPadded(uint64_t n, unsigned int padding) {
	std::string ret;
	while (n > 0) {
		ret = std::string(1, (char)(n&255)) + ret;
		n = n >> 8;
	}
	while (ret.size() < padding)
		ret = '\0' + ret;
	return ret;
}

void WhatsappConnection::sendEncrypt(bool fresh)
{
	std::cerr << "Generating keys..." << std::endl;

	IdentityKeyPair identityKeyPair = fresh ? KeyHelper::generateIdentityKeyPair() : axolotlStore->getIdentityKeyPair();
	uint64_t registrationId = fresh ? KeyHelper::generateRegistrationId() : axolotlStore->getLocalRegistrationId();
	std::vector<PreKeyRecord> preKeys = KeyHelper::generatePreKeys(KeyHelper::getRandomFFFFFFFF(), 100);
	SignedPreKeyRecord signedPreKey = KeyHelper::generateSignedPreKey(identityKeyPair, KeyHelper::getRandomFFFFFFFF() & 0xFFFF);

	Tree iq("iq", makeat({"id", getNextIqId(), "type", "set", "to", whatsappserver, "xmlns", "encrypt"}));

	Tree identity("identity");
	identity.setData(identityKeyPair.getPublicKey().serialize().substr(1));
	iq.addChild(identity);

	// STORE
	if (fresh)
		axolotlStore->storeLocalData(registrationId, identityKeyPair);

	Tree list("list");
	for (auto preKey: preKeys) {
		Tree keyNode("key");
		Tree idNode("id");
		std::string keyId = numToBytesZPadded(preKey.getId(), 3);
		idNode.setData(keyId);

		Tree valueNode("value");
		valueNode.setData(preKey.getKeyPair().getPublicKey().serialize().substr(1));
		keyNode.addChild(idNode);
		keyNode.addChild(valueNode);
		list.addChild(keyNode);

		// STORE
		axolotlStore->storePreKey(preKey.getId(), preKey);
	}
	iq.addChild(list);

	Tree registrationNode("registration");
	registrationNode.setData(numToBytesZPadded(registrationId, 4));
	iq.addChild(registrationNode);

	Tree typeNode("type");
	typeNode.setData(std::string(1, '\5'));
	iq.addChild(typeNode);

	Tree skeyNode("skey");
	Tree idNode("id");
	std::string keyId = numToBytesZPadded(signedPreKey.getId(), 3);
	idNode.setData(keyId);
	Tree valueNode("value");
	valueNode.setData(signedPreKey.getKeyPair().getPublicKey().serialize().substr(1));
	Tree signatureNode("signature");
	signatureNode.setData(signedPreKey.getSignature());
	skeyNode.addChild(idNode);
	skeyNode.addChild(valueNode);
	skeyNode.addChild(signatureNode);
	iq.addChild(skeyNode);

	// STORE
	axolotlStore->storeSignedPreKey(signedPreKey.getId(), signedPreKey);

	outbuffer = outbuffer + serialize_tree(&iq);
}

void WhatsappConnection::sendMessageRetry(const std::string &from, const std::string &msgid, unsigned long long t)
{
	Tree resp("receipt", makeat({"to", from, "id", msgid, "type", "retry", "t", std::to_string(time(0))}));

	Tree registrationNode("registration");
	uint64_t registrationId = axolotlStore->getLocalRegistrationId();
	//registrationNode.setData(std::string("\0\0\0\0", 4));
	registrationNode.setData(numToBytesZPadded(registrationId, 4));
	resp.addChild(registrationNode);

	Tree retryNode("retry", makeat({"count", "1", "id", msgid, "v", "1", "t", std::to_string(t)}));
	resp.addChild(retryNode);

	outbuffer = outbuffer + serialize_tree(&resp);
}

SessionCipher *WhatsappConnection::getSessionCipher(uint64_t recepient) {
	if (cipherHash.find(recepient) == cipherHash.end()) {
		SessionCipher *cipher = new SessionCipher(axolotlStore, recepient, 1);
		cipherHash[recepient] = cipher;
	}
	return cipherHash[recepient];
}

std::string decode7bit(std::string s) {
	if (s.size() && s[0] == '\n') {
		s = s.substr(1); // Remove first \n
		do {
			s = s.substr(1);
		} while (s.size() && (s[0] & 0x80));

		if (s.size())
			s = s.substr(0, s.size()-1);
	}
	return s;
}

bool WhatsappConnection::parsePreKeyWhisperMessage(std::string jid, std::string id,
	std::string author, unsigned long long time, Tree enc) {

	try {
		std::shared_ptr<PreKeyWhisperMessage> message(new PreKeyWhisperMessage(enc.getData()));

		uint64_t recepientId = JidAsInt(jid);
		SessionCipher *cipher = getSessionCipher(recepientId);
		std::string plaintext = decode7bit(cipher->decrypt(message));

		this->receiveMessage(ChatMessage(this, jid, time, id, plaintext, author));
	}
	catch (WhisperException &e) {
		std::cerr << "EXCEPTION " << e.errorType() << e.errorMessage() << std::endl;
		sendMessageRetry(jid, id, time);
		return false;
	}
	return true;
}

bool WhatsappConnection::parseWhisperMessage(std::string jid, std::string id,
	std::string author, unsigned long long time, Tree enc) {

	try {
		std::shared_ptr<WhisperMessage> message(new WhisperMessage(enc.getData()));

		uint64_t recepientId = JidAsInt(jid);
		SessionCipher *cipher = getSessionCipher(recepientId);
		std::string plaintext = decode7bit(cipher->decrypt(message));

		this->receiveMessage(ChatMessage(this, jid, time, id, plaintext, author));
	}
	catch (WhisperException &e) {
		std::cerr << "EXCEPTION " << e.errorType() << e.errorMessage() << std::endl;
		sendMessageRetry(jid, id, time);
		return false;
	}
	return true;
}




DataBuffer WhatsappConnection::serialize_tree(Tree * tree, bool crypt)
{
	DEBUG_PRINT( tree->toString() );

	DataBuffer data = write_tree(tree);
	if (data.size() >= 1024*1024) {
		std::cerr << "Skipping huge tree! " << data.size() << std::endl;
		return DataBuffer();
	}
	unsigned char flag = 0;
	if (crypt) {
		data = data.encodedBuffer(this->out, &this->session_key[20*1], true, this->frame_seq++);
		flag = 0x80;
	}

	DataBuffer ret;
	ret.putInt(flag | (data.size() >> 16), 1);
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
	if (tree->getData().size() != 0)
		len++;

	bout.writeListSize(len);
	if (tree->getTag() == "start")
		bout.putInt(1, 1);
	else
		bout.putString(tree->getTag());
	tree->writeAttributes(&bout);

	if (tree->getData().size() > 0)
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
	int flag = data->getInt(1);
	int bflag = (flag & 0xF0) >> 4;
	int bsize = data->getInt(2, 1) | ((flag & 0xF) << 16);
	if (bsize > data->size() - 3)
		return false; /* Next message incomplete, return consumed data */

	data->popData(3);

	if (bflag & 8) {
		/* Decode data, buffer conversion */
		if (this->in != NULL) {
			DataBuffer *decoded_data = data->decodedBuffer(this->in, bsize, false);

			bool res;
			if (bflag & 4) {
				DataBuffer *decomp_data = decoded_data->decompressedBuffer();
				if (decomp_data != NULL) {
					res = read_tree(decomp_data, t);
					delete decomp_data;
				}
				else
					res = false;
			} else {
				res = read_tree(decoded_data, t);
			}
			delete decoded_data;

			data->popData(bsize);	/* Pop data unencrypted for next parsing! */
			return res;
		} else {
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
	
	recv_messages.push_back(mc);

	DEBUG_PRINT("Received message type " << m.type() << " from " << m.from << " at " << m.t);

	/* Now add the contact in the list (to query the profile picture) */
	if (contacts.find(m.from) == contacts.end())
		contacts[m.from] = Contact(m.from, false);
	this->addContacts(std::vector < std::string > ());
}

void WhatsappConnection::notifyPresence(std::string from, std::string status, std::string last)
{
	if (status == "")
		status = "available";

	from = getusername(from);
	contacts[from].presence = status;
	if (last == "")
		contacts[from].last_seen = 0;  // Active now
	else if (last != "deny" and last != "none" and last != "error")
		contacts[from].last_seen = std::stoull(last);
	else
		contacts[from].last_seen = ~0;

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
		notifyMyMessage();
	}
}

void WhatsappConnection::notifyMyPresence()
{
	/* Send the nickname and the current status */
	Tree pres("presence", makeat({"name", nickname, "type", mypresence}));

	outbuffer = outbuffer + serialize_tree(&pres);
}

void WhatsappConnection::sendInitial()
{
	Tree conf("config");
	Tree iq("iq", makeat({"id", getNextIqId(), "type", "get", "to", whatsappserver, "xmlns", "urn:xmpp:whatsapp:push"}));
	iq.addChild(conf);	

	outbuffer = outbuffer + serialize_tree(&iq);
}

void WhatsappConnection::notifyMyMessage()
{
	/* Send the status message */
	Tree status("status");
	status.setData(this->mymessage);

	Tree mes("iq", makeat({"to", whatsappserver, "type", "set", "id", getNextIqId(), "xmlns", "status"}));
	mes.addChild(status);

	outbuffer = outbuffer + serialize_tree(&mes);
}

void WhatsappConnection::updatePrivacy(
	const std::string & show_last_seen,
	const std::string & show_profile_pic,
	const std::string & show_status_msg) {

	std::cout << "LLL " << show_last_seen << std::endl;

	Tree last   ("category", makeat({"name", "last",    "value", show_last_seen}));
	Tree profile("category", makeat({"name", "profile", "value", show_profile_pic}));
	Tree status ("category", makeat({"name", "status",  "value", show_status_msg}));

	Tree mes("iq", makeat({"to", whatsappserver, "type", "set", "id", getNextIqId(), "xmlns", "privacy"}));
	Tree priv("privacy");
	priv.addChild(last); priv.addChild(profile); priv.addChild(status);
	mes.addChild(priv);

	outbuffer = outbuffer + serialize_tree(&mes);
}

void WhatsappConnection::queryPrivacy(
	std::string & show_last_seen,
	std::string & show_profile_pic,
	std::string & show_status_msg) {

	this->updatePrivacy();

	show_last_seen = this->show_last_seen;
	show_profile_pic = this->show_profile_pic;
	show_status_msg = this->show_status_msg;
}

void WhatsappConnection::updatePrivacy() {
	Tree mes("iq", makeat({"to", whatsappserver, "type", "get", "id", getNextIqId(), "xmlns", "privacy"}));
	mes.addChild(Tree("privacy"));

	outbuffer = outbuffer + serialize_tree(&mes);
}


void WhatsappConnection::notifyError(ErrorCode err, const std::string & reason)
{
	error_queue.push_back(std::make_pair(err,reason));
}

WhatsappConnection::ErrorCode WhatsappConnection::getErrors(std::string & reason) {
	if (error_queue.size() > 0) {
		ErrorCode r = error_queue[0].first;
		reason = error_queue[0].second;
		error_queue.erase(error_queue.begin());
		return r;
	}
	return errorNoError;
}

Message* WhatsappConnection::getReceivedMessage()
{
	if (recv_messages.size()) {
		Message * ret = recv_messages[0];
		recv_messages.erase(recv_messages.begin() + 0);
		return ret;
	}
	return NULL;
}

int WhatsappConnection::getUserStatus(const std::string & who)
{
	if (contacts.find(who) != contacts.end()) {
		if (contacts[who].presence == "available")
			return 1;
		return 0;
	}
	return -1;
}

std::string WhatsappConnection::getUserStatusString(const std::string & who)
{
	if (contacts.find(who) != contacts.end()) {
		return contacts[who].status;
	}
	return "";
}

unsigned long long WhatsappConnection::getLastSeen(const std::string & who)
{
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
	Tree t("iq", makeat({"to",from, "id", id, "type","result"}));
	outbuffer = outbuffer + serialize_tree(&t);
}

void WhatsappConnection::sendResponse()
{
	Tree t("response");

	std::string response = phone + challenge_data + std::to_string(time(NULL));
	DataBuffer eresponse(response.c_str(), response.size());
	eresponse = eresponse.encodedBuffer(this->out, &this->session_key[20*1], false, this->frame_seq++);
	response = eresponse.toString();
	t.setData(response);

	outbuffer = outbuffer + serialize_tree(&t, false);
}


