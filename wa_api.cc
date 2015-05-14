
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#include <glib.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <map>
#include <stdio.h>

#include "wa_api.h"
#include "contacts.h"
#include "whatsapp_api.h"
#include "message.h"

char *waAPI_getgroups(void *waAPI)
{
	std::map < std::string, Group > g = ((WhatsappConnectionAPI *) waAPI)->getGroups();
	std::string ids;
	for (std::map < std::string, Group >::iterator it = g.begin(); it != g.end(); it++) {
		if (it != g.begin())
			ids += ",";
		ids += it->first;
	}
	return g_strdup(ids.c_str());
}

int waAPI_getgroupsupdated(void *waAPI)
{
	if (((WhatsappConnectionAPI *) waAPI)->groupsUpdated())
		return 1;
	return 0;
}

int waAPI_getgroupinfo(void *waAPI, const char *id, char **subject, char **owner, char **p)
{
	std::map < std::string, Group > ret = ((WhatsappConnectionAPI *) waAPI)->getGroups();

	std::string sid = std::string(id);
	if (ret.find(sid) == ret.end())
		return 0;

	std::string part;
	for (unsigned int i = 0; i < ret.at(sid).participants.size(); i++) {
		if (i != 0)
			part += ",";
		part += ret.at(sid).participants[i];
	}

	if (subject)
		*subject = g_strdup(ret.at(sid).subject.c_str());
	if (owner)
		*owner = g_strdup(ret.at(sid).owner.c_str());
	if (p)
		*p = g_strdup(part.c_str());

	return 1;
}

int waAPI_queryreceivedmsg(void *waAPI, char * id, int * type) {
	std::string msgid;
	if (((WhatsappConnectionAPI *) waAPI)->queryReceivedMessage(msgid, *type)) {
		strcpy(id, msgid.c_str());
		return 1;
	}
	return 0;
}

void waAPI_creategroup(void *waAPI, const char *subject)
{
	((WhatsappConnectionAPI *) waAPI)->addGroup(std::string(subject));
}

void waAPI_deletegroup(void *waAPI, const char *subject)
{
	((WhatsappConnectionAPI *) waAPI)->leaveGroup(std::string(subject));
}

void waAPI_manageparticipant(void *waAPI, const char *id, const char *part, const char *command)
{
	((WhatsappConnectionAPI *) waAPI)->manageParticipant(std::string(id), std::string(part), std::string(command));
}

void waAPI_setavatar(void *waAPI, const void *buffer, int len, const void *buffers, int lens)
{
	std::string im ((const char *)buffer,  (size_t) len);
	std::string imp((const char *)buffers, (size_t) lens);
	((WhatsappConnectionAPI *) waAPI)->send_avatar(im, imp);
}

int waAPI_sendcb(void *waAPI, void *buffer, int maxbytes)
{
	return ((WhatsappConnectionAPI *) waAPI)->sendCallback((char *)buffer, maxbytes);
}

void waAPI_senddone(void *waAPI, int bytessent)
{
	return ((WhatsappConnectionAPI *) waAPI)->sentCallback(bytessent);
}

void waAPI_input(void *waAPI, const void *buffer, int bytesrecv)
{
	((WhatsappConnectionAPI *) waAPI)->receiveCallback((char *)buffer, bytesrecv);
}

int waAPI_hasoutdata(void *waAPI)
{
	if (((WhatsappConnectionAPI *) waAPI)->hasDataToSend())
		return 1;
	return 0;
}

int waAPI_sslsendcb(void *waAPI, void *buffer, int maxbytes)
{
	return ((WhatsappConnectionAPI *) waAPI)->sendSSLCallback((char *)buffer, maxbytes);
}

void waAPI_sslsenddone(void *waAPI, int bytessent)
{
	((WhatsappConnectionAPI *) waAPI)->sentSSLCallback(bytessent);
}

void waAPI_sslinput(void *waAPI, const void *buffer, int bytesrecv)
{
	((WhatsappConnectionAPI *) waAPI)->receiveSSLCallback((char *)buffer, bytesrecv);
}

int waAPI_sslhasoutdata(void *waAPI)
{
	if (((WhatsappConnectionAPI *) waAPI)->hasSSLDataToSend())
		return 1;
	if (((WhatsappConnectionAPI *) waAPI)->closeSSLConnection())
		return -1;
	return 0;
}

int waAPI_hassslconnection(void *waAPI, char **host, int *port)
{
	std::string shost;
	bool r = ((WhatsappConnectionAPI *) waAPI)->hasSSLConnection(shost, port);
	if (r)
		*host = (char *)g_strdup(shost.c_str());
	return r;
}

void waAPI_sslcloseconnection(void *waAPI)
{
	((WhatsappConnectionAPI *) waAPI)->SSLCloseCallback();
}

void waAPI_login(void *waAPI, const char *ua)
{
	((WhatsappConnectionAPI *) waAPI)->doLogin(std::string(ua));
}

void *waAPI_create(const char *username, const char *password, const char *nickname)
{
	WhatsappConnectionAPI *api = new WhatsappConnectionAPI(std::string(username), std::string(password), std::string(nickname));
	return api;
}

void waAPI_delete(void *waAPI)
{
	delete((WhatsappConnectionAPI *) waAPI);
}

void waAPI_sendim(void *waAPI, const char *id, const char *who, const char *message)
{
	((WhatsappConnectionAPI *) waAPI)->sendChat(id, who, message);
}

void waAPI_sendchat(void *waAPI, const char *id, const char *who, const char *message)
{
	((WhatsappConnectionAPI *) waAPI)->sendGroupChat(id, who, message);
}

int waAPI_sendimage(void *waAPI, const char *who, int w, int h, unsigned int size, const char *fp)
{
	return ((WhatsappConnectionAPI *) waAPI)->sendImage(std::string(who), w, h, size, fp);
}

int waAPI_fileuploadprogress(void *waAPI, int *rid, int *bs)
{
	int ridl, bsl;
	int r = ((WhatsappConnectionAPI *) waAPI)->uploadProgress(ridl, bsl);
	*rid = ridl;
	*bs = bsl;
	return r;
}

int waAPI_fileuploadcomplete(void *waAPI, int rid)
{
	return ((WhatsappConnectionAPI *) waAPI)->uploadComplete(rid);
}

void waAPI_sendtyping(void *waAPI, const char *who, int typing)
{
	((WhatsappConnectionAPI *) waAPI)->notifyTyping(std::string(who), typing);
}

void waAPI_accountinfo(void *waAPI, unsigned long long *creation, unsigned long long *freeexpires, char **status)
{
	std::string st;
	unsigned long long cr, fe;
	((WhatsappConnectionAPI *) waAPI)->account_info(cr, fe, st);
	*creation = cr;
	*freeexpires = fe;
	*status = g_strdup(st.c_str());
}

void waAPI_queryprivacy(void *waAPI, char * last, char * profile, char* status) {
	std::string slast, sprofile, sstatus;
	((WhatsappConnectionAPI *) waAPI)->queryPrivacy(slast, sprofile, sstatus);

	strcpy(last, slast.c_str());
	strcpy(profile, sprofile.c_str());
	strcpy(status, sstatus.c_str());
}

void waAPI_setprivacy(void *waAPI, const char * last, const char * profile, const char* status) {
	((WhatsappConnectionAPI *) waAPI)->updatePrivacy(last, profile, status);
}

int waAPI_querymsg(void *waAPI, t_message * msg) {
	WhatsappConnectionAPI * wa = (WhatsappConnectionAPI *)waAPI;

	Message * m = wa->getReceivedMessage();
	if (m == NULL) return 0;

	memset(msg,0,sizeof(t_message));
	msg->type   = m->type();
	msg->who    = g_strdup(m->from.c_str());
	msg->author = g_strdup(m->author.c_str());
	msg->t      = m->t;

	if (msg->type == CHAT_MESSAGE)
		msg->message = g_strdup(((ChatMessage*)m)->message.c_str());
	if (msg->type == IMAGE_MESSAGE || msg->type == SOUND_MESSAGE || msg->type == VIDEO_MESSAGE)
		msg->url = g_strdup(((MediaMessage*)m)->url.c_str());
	if (msg->type == IMAGE_MESSAGE) {
		std::string r = (((ImageMessage*)m)->preview);
		msg->image = (char*)g_memdup(r.c_str(), r.size());
		msg->imagelen = r.size();
	}
	if (msg->type == LOCAT_MESSAGE) {
		msg->lat = ((LocationMessage*)m)->latitude;
		msg->lng = ((LocationMessage*)m)->longitude;
	}

	delete m;

	return 1;
}

int waAPI_querystatus(void *waAPI, char **who, int *stat)
{
	std::string f;
	int st;
	if (((WhatsappConnectionAPI *) waAPI)->query_status(f, st)) {
		*who = g_strdup(f.c_str());
		*stat = st;
		return 1;
	}
	return 0;
}

int waAPI_getuserstatus(void *waAPI, const char *who)
{
	return ((WhatsappConnectionAPI *) waAPI)->getuserstatus(std::string(who));
}

int waAPI_geterror(void *waAPI, char ** reason)
{
	std::string sreason;
	int r = (int)((WhatsappConnectionAPI *) waAPI)->getErrors(sreason);
	if (r != 0)
		*reason = g_strdup(sreason.c_str());
	return r;
}

char *waAPI_getuserstatusstring(void *waAPI, const char *who)
{
	if (!waAPI)
		return 0;
	std::string s = ((WhatsappConnectionAPI *) waAPI)->getuserstatusstring(std::string(who));
	return g_strdup(s.c_str());
}

unsigned long long waAPI_getlastseen(void *waAPI, const char *who)
{
	return ((WhatsappConnectionAPI *) waAPI)->getlastseen(std::string(who));
}

int waAPI_queryicon(void *waAPI, char **who, char **icon, int *len, char **hash)
{
	std::string f, ic, hs;
	if (((WhatsappConnectionAPI *) waAPI)->query_icon(f, ic, hs)) {
		*who = g_strdup(f.c_str());
		*icon = (char *)g_memdup(ic.c_str(), ic.size());
		*len = ic.size();
		*hash = g_strdup(hs.c_str());
		return 1;
	}
	return 0;
}

int waAPI_queryavatar(void *waAPI, const char *who, char **icon, int *len)
{
	std::string ic;
	if (((WhatsappConnectionAPI *) waAPI)->query_avatar(std::string(who), ic)) {
		*icon = (char *)g_memdup(ic.c_str(), ic.size());
		*len = ic.size();
		return 1;
	}
	return 0;
}

int waAPI_querytyping(void *waAPI, char **who, int *stat)
{
	std::string f;
	int status;
	if (((WhatsappConnectionAPI *) waAPI)->query_typing(f, status)) {
		*who = g_strdup(f.c_str());
		*stat = status;
		return 1;
	}
	return 0;
}

int waAPI_loginstatus(void *waAPI)
{
	return ((WhatsappConnectionAPI *) waAPI)->loginStatus();
}

void waAPI_getmsgid(void *waAPI, char * msgid) {
	((WhatsappConnectionAPI *) waAPI)->getMessageId(msgid);
}

void waAPI_addcontact(void *waAPI, const char *phone)
{
	std::vector < std::string > clist;
	clist.push_back(std::string(phone));
	((WhatsappConnectionAPI *) waAPI)->addContacts(clist);
}

void waAPI_contactsupdate(void *waAPI)
{
	((WhatsappConnectionAPI *) waAPI)->contactsUpdate();
}

void waAPI_delcontact(void *waAPI, const char *phone)
{

}

void waAPI_setmypresence(void *waAPI, const char *st, const char *msg)
{
	((WhatsappConnectionAPI *) waAPI)->setMyPresence(st, msg);
}


