/**
 * purple
 *
 * Purple is the legal property of its developers, whose names are too numerous
 * to list here.  Please refer to the COPYRIGHT file distributed with this
 * source distribution.
 *
 * Whatsapp is a free implementation of the WhatsApp protocol for libpurple.
 * The implementation is not 100% complete. Currently supported features include
 * message send and receive and profile pictures. In order to be able to login 
 * you need your WhatsApp password, which is not easy to know. As of Jun. 2012
 * the password was either the IMEI or the MAC addres, but latest versions
 * of the protocol changed the password so now it's server-generated.
 * For more info check WhatsAPI (https://github.com/venomous0x/WhatsAPI)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301  USA
 */

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <fstream>
#include <math.h>

#include "account.h"
#include "accountopt.h"
#include "blist.h"
#include "cmds.h"
#include "conversation.h"
#include "connection.h"
#include "debug.h"
#include "notify.h"
#include "privacy.h"
#include "prpl.h"
#include "roomlist.h"
#include "status.h"
#include "util.h"
#include "wa_util.h"
#include "version.h"
#include "request.h"

#include "wa_connection.h"
#include "message.h"
#include "imgutil.h"
#include "wa_constants.h"

#ifdef _WIN32
#define sys_read  wpurple_read
#define sys_write wpurple_write
#define sys_close wpurple_close
extern "C" {
  int wpurple_close(int fd);
  int wpurple_write(int fd, const void *buf, unsigned int size);
  int wpurple_read(int fd, void *buf, unsigned int size);
}
#else
#include <unistd.h>
#define sys_read  read
#define sys_write write
#define sys_close close
#endif

const char default_resource[] = WHATSAPP_VERSION;

#define WHATSAPP_ID "whatsapp"
extern "C" {
	static PurplePlugin *_whatsapp_protocol = NULL;
}



typedef struct {
	bool upload;
	unsigned int file_size;
	char *to;
	void *wconn;
	PurpleConnection *gc;
	int ref_id;
	int done, started;
	std::string url, aeskey, iv;
} wa_file_transfer;

typedef struct {
	PurpleAccount *account;
	int fd;			/* File descriptor of the socket */
	guint rh, wh;		/* Read/write handlers */
	guint timer;        /* Keep alive timer */
	int connected;		/* Connection status */
	WhatsappConnection *waAPI;		/* Pointer to the C++ class which actually implements the protocol */
	int conv_id;		/* Combo id counter */
	/* HTTPS interface for status query */
	guint sslrh, sslwh;	/* Read/write handlers */
	int sslfd;
	PurpleSslConnection *gsc;	/* SSL handler */
} whatsapp_connection;

static void waprpl_check_output(PurpleConnection * gc);
static void waprpl_process_incoming_events(PurpleConnection * gc);
static void waprpl_insert_contacts(PurpleConnection * gc);
static void waprpl_chat_join(PurpleConnection * gc, GHashTable * data);
void check_ssl_requests(PurpleAccount * acct);
void waprpl_ssl_cerr_cb(PurpleSslConnection * gsc, PurpleSslErrorType error, gpointer data);
void waprpl_check_ssl_output(PurpleConnection * gc);
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond);
static void waprpl_set_status(PurpleAccount * acct, PurpleStatus * status);
static void waprpl_check_complete_uploads(PurpleConnection * gc);
void waprpl_image_download_offer(PurpleConnection *, std::string, std::string, bool, std::string, std::string);

unsigned int chatid_to_convo(const char *id)
{
	/* Get the chat number to use as combo id */
	int unused, cid;
	sscanf(id, "%d-%d", &unused, &cid);
	return cid;
}

static void waprpl_tooltip_text(PurpleBuddy * buddy, PurpleNotifyUserInfo * info, gboolean full)
{
	const char *status;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
	int st = wconn->waAPI->getUserStatus(purple_buddy_get_name(buddy));
	if (st < 0)
		status = "Unknown";
	else if (st == 0)
		status = "Unavailable";
	else
		status = "Available";
	unsigned long long lseen = wconn->waAPI->getLastSeen(purple_buddy_get_name(buddy));
	std::string statusmsg = wconn->waAPI->getUserStatusString(purple_buddy_get_name(buddy));
	purple_notify_user_info_add_pair_plaintext(info, "Status", status);
	if (lseen == 0)
		purple_notify_user_info_add_pair_plaintext(info, "Last seen on WhatsApp", "Now");
	else if (lseen == ~0)
		purple_notify_user_info_add_pair_plaintext(info, "Last seen on WhatsApp", "N/A");
	else
		purple_notify_user_info_add_pair_plaintext(info, "Last seen on WhatsApp", purple_str_seconds_to_string(time(0) - lseen));
	purple_notify_user_info_add_pair_plaintext(info, "Status message", g_strdup(statusmsg.c_str()));
}

static char *waprpl_status_text(PurpleBuddy * buddy)
{
	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
	if (!wconn)
		return 0;

	std::string statusmsg = wconn->waAPI->getUserStatusString(purple_buddy_get_name(buddy));
	if (statusmsg == "")
		return NULL;
	return g_strdup(statusmsg.c_str());
}

static const char *waprpl_list_icon(PurpleAccount * acct, PurpleBuddy * buddy)
{
	return "whatsapp";
}

/* Show the account information received at the login such as expiration,
 * creation, etc. */
static void waprpl_show_accountinfo(PurplePluginAction * action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(gc);
	if (!wconn)
		return;

	unsigned long long creation, freeexpires;
	std::string status;
	wconn->waAPI->account_info(creation, freeexpires, status);

	time_t creationtime = creation;
	time_t freeexpirestime = freeexpires;
	char *cr = g_strdup(asctime(localtime(&creationtime)));
	char *ex = g_strdup(asctime(localtime(&freeexpirestime)));
	char *text = g_strdup_printf("Account status: %s<br />Created on: %s<br />Free expires on: %s\n", status.c_str(), cr, ex);

	purple_notify_formatted(gc, "Account information", "Account information", "", text, NULL, NULL);

	g_free(text);
	g_free(ex);
	g_free(cr);
}

const char * priv_opt[3]      = {"all", "contacts", "none" };
const char * priv_opt_nice[3] = {"Everybody", "Only contacts", "No one" };

const char * priv_type[3]      = {"last", "profile", "status" };
const char * priv_type_nice[3] = {"Last seen", "Profile picture", "Status message" };

static void waprpl_update_privacy(PurpleConnection *gc, PurpleRequestFields *fields) {
	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(gc);

	int i,j;
	char priv[3][30];
	for (i = 0; i < 3; i++) {
		PurpleRequestField * field = purple_request_fields_get_field(fields, priv_type[i]);
		GList *sel = purple_request_field_list_get_selected (field);
		for (j = 0; j < 3; j++)
			if (strcmp((char*)sel->data, priv_opt_nice[j]) == 0)
				strcpy(priv[i], priv_opt[j]);
	}

	wconn->waAPI->updatePrivacy(priv[0], priv[1], priv[2]);
	waprpl_check_output(gc);
}

/* Show privacy settings */
static void waprpl_show_privacy(PurplePluginAction * action)
{
	PurpleConnection *gc = (PurpleConnection *) action->context;
	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(gc);
	if (!wconn)
		return;

	std::vector <std::string> priv(3);
	wconn->waAPI->queryPrivacy(priv[0], priv[1], priv[2]);

	PurpleRequestField *field;

	PurpleRequestFields *fields = purple_request_fields_new();
	PurpleRequestFieldGroup *group = purple_request_field_group_new(NULL);
	purple_request_fields_add_group(fields, group);

	int i,j;
	for (j = 0; j < 3; j++) {
		field = purple_request_field_list_new(priv_type[j], priv_type_nice[j]);
		for (i = 0; i < 3; i++) {
			purple_request_field_list_add(field, priv_opt_nice[i], g_strdup(priv_opt[i]));
			if (strcmp(priv_opt[i], priv[j].c_str()) == 0)
				purple_request_field_list_add_selected(field, priv_opt_nice[i]);
		}
		purple_request_field_group_add_field(group, field);
	}

	purple_request_fields(gc, "Edit privacy settings", "Edit privacy settings",
						NULL, fields, 
						"Save", G_CALLBACK(waprpl_update_privacy),
						"Cancel", NULL,
						purple_connection_get_account(gc), NULL, NULL,
						gc);
}

static GList *waprpl_actions(PurplePlugin * plugin, gpointer context)
{
	PurplePluginAction *act;
	GList *actions = NULL;

	act = purple_plugin_action_new("Show account information ...", waprpl_show_accountinfo);
	actions = g_list_append(actions, act);

	act = purple_plugin_action_new("Set privacy ...", waprpl_show_privacy);
	actions = g_list_append(actions, act);

	return actions;
}

static bool isgroup(std::string user)
{
	return user.find("-") != std::string::npos;
}

static void waprpl_blist_node_removed(PurpleBlistNode * node)
{
	if (!PURPLE_BLIST_NODE_IS_CHAT(node))
		return;

	PurpleChat *ch = PURPLE_CHAT(node);
	PurpleConnection *gc = purple_account_get_connection(purple_chat_get_account(ch));
	if (purple_connection_get_prpl(gc) != _whatsapp_protocol)
		return;

	char *gid = (char*)g_hash_table_lookup(purple_chat_get_components(ch), "id");
	if (gid == 0)
		return;		/* Group is not created yet... */
	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(gc);
	wconn->waAPI->leaveGroup(gid);
	waprpl_check_output(purple_account_get_connection(purple_chat_get_account(ch)));
}

static void waprpl_blist_node_added(PurpleBlistNode * node)
{
	if (!PURPLE_BLIST_NODE_IS_CHAT(node))
		return;

	PurpleChat *ch = PURPLE_CHAT(node);
	PurpleConnection *gc = purple_account_get_connection(purple_chat_get_account(ch));
	if (purple_connection_get_prpl(gc) != _whatsapp_protocol)
		return;

	whatsapp_connection *wconn = (whatsapp_connection *)purple_connection_get_protocol_data(gc);
	GHashTable *hasht = purple_chat_get_components(ch);
	const char *groupname = (char*)g_hash_table_lookup(hasht, "subject");
	const char *gid = (char*)g_hash_table_lookup(hasht, "id");
	if (gid != 0)
		return;		/* Already created */
	purple_debug_info(WHATSAPP_ID, "Creating group %s\n", groupname);

	wconn->waAPI->addGroup(groupname);
	waprpl_check_output(purple_account_get_connection(purple_chat_get_account(ch)));

	/* Remove it, it will get added at the moment the chat list gets refreshed */
	purple_blist_remove_chat(ch);
}

static PurpleChat *blist_find_chat_by_hasht_cond(PurpleConnection *gc, int (*fn)(GHashTable *hasht, void *data), void *data)
{
	PurpleAccount *account = purple_connection_get_account(gc);
	PurpleBlistNode *node = purple_blist_get_root();
	GHashTable *hasht;

	while (node) {
		if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
			PurpleChat *ch = PURPLE_CHAT(node);
			if (purple_chat_get_account(ch) == account) {
				hasht = purple_chat_get_components(ch);
				if (fn(hasht, data))
					return ch;
			}
		}
		node = purple_blist_node_next(node, FALSE);
	}

	return NULL;
}

static int hasht_cmp_id(GHashTable *hasht, void *data)
{
	return !strcmp((char*)g_hash_table_lookup(hasht, "id"), *((char **)data));
}

static int hasht_cmp_convo(GHashTable *hasht, void *data)
{
	return (chatid_to_convo((char*)g_hash_table_lookup(hasht, "id")) == *((int *)data));
}

static PurpleChat *blist_find_chat_by_id(PurpleConnection *gc, const char *id)
{
	return blist_find_chat_by_hasht_cond(gc, hasht_cmp_id, &id);
}

static PurpleChat *blist_find_chat_by_convo(PurpleConnection *gc, int convo)
{
	return blist_find_chat_by_hasht_cond(gc, hasht_cmp_convo, &convo);
}

static PurpleChat * create_chat_group(const char * gpid, whatsapp_connection *wconn, PurpleAccount *acc) {
	purple_debug_info(WHATSAPP_ID, "Creating new group: %s\n", gpid);

	std::string subject = "Unknown", owner = "00000", part, admins = "00000";
	std::map < std::string, Group > glist = wconn->waAPI->getGroups();
	if (glist.find(gpid) != glist.end()) {
		subject = glist.at(gpid).subject;
		owner   = glist.at(gpid).owner;
		admins  = glist.at(gpid).getAdminList();
	}

	GHashTable *htable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	g_hash_table_insert(htable, g_strdup("subject"), g_strdup(subject.c_str()));
	g_hash_table_insert(htable, g_strdup("id"), g_strdup(gpid));
	g_hash_table_insert(htable, g_strdup("owner"), g_strdup(owner.c_str()));
	g_hash_table_insert(htable, g_strdup("admins"), g_strdup(admins.c_str()));

	PurpleChat * ch = purple_chat_new(acc, subject.c_str(), htable);
	purple_blist_add_chat(ch, NULL, NULL);

	return ch;
}

static void conv_add_participants(PurpleConversation * conv, const char *part, const char *owner, const char * admins)
{
	gchar **plist = g_strsplit(part, ",", 0);
	gchar **alist = g_strsplit(admins, ",", 0);
	gchar **p;

	purple_conv_chat_clear_users(purple_conversation_get_chat_data(conv));
	for (p = plist; *p; p++) {
		PurpleConvChatBuddyFlags flags = (!strcmp(owner, *p) ? PURPLE_CBFLAGS_FOUNDER : PURPLE_CBFLAGS_NONE);
		gchar **p2;
		for (p2 = alist; *p2; p2++)
			if (!strcmp(*p2, *p) && flags == PURPLE_CBFLAGS_NONE)
				flags = PURPLE_CBFLAGS_OP;
		purple_conv_chat_add_user(purple_conversation_get_chat_data(conv), *p, "", flags, FALSE);
	}

	g_strfreev(plist);
	g_strfreev(alist);
}

PurpleConversation *get_open_combo(const char *who, PurpleConnection * gc)
{
	PurpleAccount *acc = purple_connection_get_account(gc);
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	purple_debug_info(WHATSAPP_ID, "Opening conversation window for %s\n", who);

	if (isgroup(who)) {
		/* Search fot the combo */
		PurpleChat *ch = blist_find_chat_by_id(gc, who);
		if (!ch)
			ch = create_chat_group(who, wconn, acc);

		GHashTable *hasht = purple_chat_get_components(ch);
		int convo_id = chatid_to_convo(who);
		const char *groupname = (char*)g_hash_table_lookup(hasht, "subject");
		PurpleConversation *convo = purple_find_chat(gc, convo_id);
		
		/* Create a window if it's not open yet */
		if (!convo) {
			waprpl_chat_join(gc, hasht);
			convo = purple_find_chat(gc, convo_id);
		}
		else if (purple_conv_chat_has_left(PURPLE_CONV_CHAT(convo))) {
			std::map < std::string, Group > glist = wconn->waAPI->getGroups();
			if (glist.find(who) != glist.end()) {
				convo = serv_got_joined_chat(gc, convo_id, groupname);
				purple_debug_info(WHATSAPP_ID, "group info ID(%s) SUBJECT(%s) OWNER(%s)\n",
					who, glist.at(who).subject.c_str(), glist.at(who).owner.c_str());
				conv_add_participants(convo, glist.at(who).getParticipantsList().c_str(),
					glist.at(who).owner.c_str(), glist.at(who).getAdminList().c_str());
			}
		}
		
		return convo;
	} else {
		/* Search for the combo */
		PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, acc);
		if (!convo)
			convo = purple_conversation_new(PURPLE_CONV_TYPE_IM, acc, who);
		return convo;
	}
}

static void conv_add_message(PurpleConnection * gc, const char *who, const char *msg, const char *author, unsigned long timestamp)
{
	if (isgroup(who)) {
		PurpleConversation *convo = get_open_combo(who, gc);
		if (convo)
			serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), author, PURPLE_MESSAGE_RECV, msg, timestamp);
	} else {
		serv_got_im(gc, who, msg, (PurpleMessageFlags)(PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_IMAGES), timestamp);
	}
}

static char * dbl2str(double num) {
	double a,b;
	b = modf (num, &a);
	return g_strdup_printf("%d.%08d", (int)a, (int)(b*100000000.0f));
}

static int str_array_find(gchar **haystack, const gchar *needle)
{
	int i;

	for (i = 0; haystack[i]; i++) {
		if (!strcmp(haystack[i], needle))
			return i;
	}

	return -1;
}

static void query_status(PurpleConnection *gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);
	std::string who;
	int status;

	while (wconn->waAPI->query_status(who, status)) {
		if (status == 1) {
			purple_prpl_got_user_status(acc, who.c_str(), "available", "message", "", NULL);
		} else {
			purple_prpl_got_user_status(acc, who.c_str(), "unavailable", "message", "", NULL);
		}
	}
}

static void query_typing(PurpleConnection *gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	std::string who;
	int status;

	while (wconn->waAPI->query_typing(who, status)) {
		if (status == 1) {
			purple_debug_info(WHATSAPP_ID, "%s is typing\n", who.c_str());
			if (!isgroup(who))
				serv_got_typing(gc, who.c_str(), 0, PURPLE_TYPING);
		} else {
			purple_debug_info(WHATSAPP_ID, "%s is not typing\n", who.c_str());
			if (!isgroup(who.c_str())) {
				serv_got_typing(gc, who.c_str(), 0, PURPLE_NOT_TYPING);
				serv_got_typing_stopped(gc, who.c_str());
			}
		}
	}
}

static void query_icon(PurpleConnection *gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);
	std::string who, icon, hash;
	int len;

	while (wconn->waAPI->query_icon(who, icon, hash)) {
		purple_debug_info(WHATSAPP_ID, "Updating user profile picture for %s\n", who.c_str());
		purple_buddy_icons_set_for_user(acc, who.c_str(), g_memdup(icon.c_str(), icon.size()), icon.size(), hash.c_str());
	}
}

static void waprpl_process_incoming_events(PurpleConnection * gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);

	WhatsappConnection::ErrorCode err;
	do {
		std::string reason;
		err = wconn->waAPI->getErrors(reason);
		if (err != WhatsappConnection::ErrorCode::errorNoError) {
			PurpleConnectionError errcode = PURPLE_CONNECTION_ERROR_OTHER_ERROR;
			if (err == WhatsappConnection::ErrorCode::errorAuth)
				errcode = PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED;
			purple_connection_error_reason(gc, errcode, reason.c_str());
		}
	} while (err != WhatsappConnection::ErrorCode::errorNoError);

	switch (wconn->waAPI->loginStatus()) {
	case 0:
		purple_connection_update_progress(gc, "Connecting", 0, 4);
		break;
	case 1:
		purple_connection_update_progress(gc, "Sending authorization", 1, 4);
		break;
	case 2:
		purple_connection_update_progress(gc, "Awaiting response", 2, 4);
		break;
	case 3:
		if (!wconn->connected) {
			purple_connection_update_progress(gc, "Connection established", 3, 4);
			purple_connection_set_state(gc, PURPLE_CONNECTED);

			PurpleAccount *account = purple_connection_get_account(gc);
			PurpleStatus *status = purple_account_get_active_status(account);

			waprpl_insert_contacts(gc);
			waprpl_set_status(account, status);

			wconn->connected = 1;
		}
		break;
	default:
		break;
	};
	
	/* Groups update */
	if (wconn->waAPI->groupsUpdated()) {
		purple_debug_info(WHATSAPP_ID, "Receiving update information from my groups\n");

		/* Delete/update the chats that are in our list */
		PurpleBlistNode *node;

		std::map < std::string, Group > glist = wconn->waAPI->getGroups();

		for (node = purple_blist_get_root(); node; node = purple_blist_node_next(node, FALSE)) {
			if (!PURPLE_BLIST_NODE_IS_CHAT(node))
				continue;

			PurpleChat *ch = PURPLE_CHAT(node);
			if (purple_chat_get_account(ch) != acc)
				continue;

			GHashTable *hasht = purple_chat_get_components(ch);
			char *grid = (char*)g_hash_table_lookup(hasht, "id");

			if (glist.find(grid) != glist.end()) {
				/* The group is in the system, update the fields */
				Group gg = glist.at(grid);
				g_hash_table_replace(hasht, g_strdup("subject"), g_strdup(gg.subject.c_str()));
				g_hash_table_replace(hasht, g_strdup("owner"), g_strdup(gg.owner.c_str()));
				purple_blist_alias_chat(ch, g_strdup(gg.subject.c_str()));
			} else {
				/* The group was deleted */
				PurpleChat *del = (PurpleChat *) node;
				node = purple_blist_node_next(node, FALSE);
				purple_blist_remove_chat(del);
			}
		}

		/* Add new groups */
		for (auto & it: glist) {
			std::string gpid = it.first;
			PurpleChat *ch = blist_find_chat_by_id(gc, gpid.c_str());
			if (!ch)
				ch = create_chat_group(gpid.c_str(), wconn, acc);
			
			/* Now update the open conversation that may exist */
			char *id = (char*)g_hash_table_lookup(purple_chat_get_components(ch), "id");
			int prplid = chatid_to_convo(id);
			PurpleConversation *conv = purple_find_chat(gc, prplid);
			if (conv) {
				conv_add_participants(conv, it.second.getParticipantsList().c_str(), it.second.owner.c_str(), it.second.getAdminList().c_str());
			}
		}
	}

	Message * m = wconn->waAPI->getReceivedMessage();
	while (m) {
		switch (m->type()) {
		case CHAT_MESSAGE: {
			ChatMessage * cm = dynamic_cast<ChatMessage*>(m);
			purple_debug_info(WHATSAPP_ID, "Got chat message from %s: %s\n", m->from.c_str(), cm->message.c_str());
			conv_add_message(gc, m->from.c_str(), cm->message.c_str(), m->author.c_str(), m->t);
			} break;
		case IMAGE_MESSAGE: {
			ImageMessage * im = dynamic_cast<ImageMessage*>(m);
			purple_debug_info(WHATSAPP_ID, "Got image from %s: %s\n", m->from.c_str(), im->url.c_str());
			int imgid = purple_imgstore_add_with_id(g_memdup(im->preview.c_str(), im->preview.size()), im->preview.size(), NULL);
			if (!im->e2e_key.size()) {
				char *msg = g_strdup_printf("<a href=\"%s\"><img id=\"%u\"></a><br/><a href=\"%s\">%s</a><br />%s",
					im->url.c_str(), imgid, im->url.c_str(), im->url.c_str(), im->caption.c_str());
				conv_add_message(gc, m->from.c_str(), msg, m->author.c_str(), m->t);
				g_free(msg);
			}else{
				std::string url = "https://davidgf.net/whatsapp/imgdec.php?url=" + im->url + "&iv=" + tohex(im->e2e_iv.c_str(), 16) + "&key=" + tohex(im->e2e_aeskey.c_str(), 32);
				std::cout << tohex(im->e2e_aeskey.c_str(), 32) << std::endl;

				char *msg = g_strdup_printf("<a href=\"%s\"><img id=\"%u\"></a><br/><a href=\"%s\">%s</a><br />%s",
					url.c_str(), imgid, url.c_str(), url.c_str(), im->caption.c_str());
				conv_add_message(gc, m->from.c_str(), msg, m->author.c_str(), m->t);
				g_free(msg);
			}

			// Offer file transfer if the user said so!
			if (true)
				waprpl_image_download_offer(gc, m->from, im->url, im->e2e_key.size() != 0, im->e2e_iv, im->e2e_aeskey);

			} break;
		case LOCAT_MESSAGE: {
			LocationMessage * lm = dynamic_cast<LocationMessage*>(m);
			purple_debug_info(WHATSAPP_ID, "Got geomessage from: %s Coordinates (%f %f)\n", 
				m->from.c_str(), (float)lm->latitude, (float)lm->longitude);
			char * lat = dbl2str(lm->latitude);
			char * lng = dbl2str(lm->longitude);
			int imgid = purple_imgstore_add_with_id(g_memdup(lm->preview.c_str(), lm->preview.size()), lm->preview.size(), NULL);
			char *msg = g_strdup_printf("<img id=\"%u\"><br />[%s]<br /><a href=\"http://openstreetmap.org/?mlat=%s&mlon=%s&zoom=20\">"
				"http://openstreetmap.org/?lat=%s&lon=%s&zoom=20</a>", 
				imgid, lm->name.c_str(), lat, lng, lat, lng);
			conv_add_message(gc, m->from.c_str(), msg, m->author.c_str(), m->t);
			g_free(msg);
			} break;
		case SOUND_MESSAGE: {
			SoundMessage * sm = dynamic_cast<SoundMessage*>(m);
			purple_debug_info(WHATSAPP_ID, "Got chat sound from %s: %s\n", m->from.c_str(), sm->url.c_str());
			char *msg = g_strdup_printf("<a href=\"%s\">%s</a>", sm->url.c_str(), sm->url.c_str());
			conv_add_message(gc, m->from.c_str(), msg, m->author.c_str(), m->t);
			g_free(msg);
			} break;
		case VIDEO_MESSAGE: {
			VideoMessage * vm = dynamic_cast<VideoMessage*>(m);
			purple_debug_info(WHATSAPP_ID, "Got chat video from %s: %s\n", m->from.c_str(), vm->url.c_str());
			char *msg = g_strdup_printf("<a href=\"%s\">%s</a><br />%s", vm->url.c_str(), vm->url.c_str(), vm->caption.c_str());
			conv_add_message(gc, m->from.c_str(), msg, m->author.c_str(), m->t);
			g_free(msg);
			} break;
		case CALL_MESSAGE: {
			purple_debug_info(WHATSAPP_ID, "Got phone call from %s\n", m->from.c_str());
			conv_add_message(gc, m->from.c_str(), "[Trying to voice-call you]", m->author.c_str(), m->t);
			} break;
		default:
			purple_debug_info(WHATSAPP_ID, "Got an unrecognized message!\n");
			break;
		};
		
		m = wconn->waAPI->getReceivedMessage();
	}

	while (1) {
		unsigned long long t;
		int typer;
		std::string msgid, from;
		if (!wconn->waAPI->queryReceivedMessage(msgid, typer, t, from))
			break;

		purple_debug_info(WHATSAPP_ID, "Received message %s type: %d (from %s)\n", msgid.c_str(), typer, from.c_str());
		purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-message-received", gc, msgid.c_str(), typer);
	}

	/* Status changes, typing notices and profile pictures. */
	query_status(gc);
	query_typing(gc);
	query_icon(gc);
}

static void waprpl_output_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		int datatosend = wconn->waAPI->sendCallback(tempbuff, sizeof(tempbuff));
		if (datatosend == 0)
			break;

		ret = sys_write(wconn->fd, tempbuff, datatosend);

		if (ret > 0) {
			wconn->waAPI->sentCallback(ret);
		} else if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
			/* Check later */
		} else {
			gchar *tmp = g_strdup_printf("Lost connection with server (out cb): %s", g_strerror(errno));
			purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
			g_free(tmp);
			break;
		}
	} while (ret > 0);

	/* Check if we need to callback again or not */
	waprpl_check_output(gc);
}

/* Try to read some data and push it to the WhatsApp API */
static void waprpl_input_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		ret = sys_read(wconn->fd, tempbuff, sizeof(tempbuff));
		if (ret > 0)
			wconn->waAPI->receiveCallback(tempbuff, ret);
		else if (ret < 0 && errno == EAGAIN)
			break;
		else if (ret < 0) {
			gchar *tmp = g_strdup_printf("Lost connection with server (in cb): %s", g_strerror(errno));
			purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
			g_free(tmp);
			break;
		} else {
			purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Server closed the connection");
		}
	} while (ret > 0);

	waprpl_process_incoming_events(gc);
	waprpl_check_output(gc);	/* The input data may generate responses! */
}

/* Checks if the WA protocol has data to output and schedules a write handler */
static void waprpl_check_output(PurpleConnection * gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	if (wconn->fd < 0)
		return;

	if (wconn->waAPI->hasDataToSend()) {
		/* Need to watch for output data (if we are not doing it already) */
		if (wconn->wh == 0)
			wconn->wh = purple_input_add(wconn->fd, PURPLE_INPUT_WRITE, waprpl_output_cb, gc);
	} else {
		if (wconn->wh != 0)
			purple_input_remove(wconn->wh);

		wconn->wh = 0;
	}

	check_ssl_requests(purple_connection_get_account(gc));
	
	waprpl_check_complete_uploads(gc);
}

static gboolean wa_timer_cb(gpointer data) {
	PurpleConnection * gc = (PurpleConnection*)data;
	waprpl_check_output(gc);

	return TRUE;
}

static void waprpl_connect_cb(gpointer data, gint source, const gchar * error_message)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	PurpleAccount *acct = purple_connection_get_account(gc);
	const char *resource = purple_account_get_string(acct, "resource", default_resource);
	gboolean send_ciphered = purple_account_get_bool(acct, "send_ciphered", TRUE);

	if (source < 0) {
		gchar *tmp = g_strdup_printf("Unable to connect: %s", error_message);
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		g_free(tmp);
	} else {
		wconn->fd = source;
		wconn->waAPI->doLogin(resource, send_ciphered);
		wconn->rh = purple_input_add(wconn->fd, PURPLE_INPUT_READ, waprpl_input_cb, gc);
		wconn->timer = purple_timeout_add_seconds(20, wa_timer_cb, gc);

		waprpl_check_output(gc);
	}
}

static void waprpl_login(PurpleAccount * acct)
{
	PurpleConnection *gc = purple_account_get_connection(acct);

	purple_debug_info(WHATSAPP_ID, "logging in %s\n", purple_account_get_username(acct));

	purple_connection_update_progress(gc, "Connecting", 0, 4);

	whatsapp_connection *wconn = (whatsapp_connection*)g_new0(whatsapp_connection, 1);
	wconn->fd = -1;
	wconn->sslfd = -1;
	wconn->account = acct;
	wconn->rh = 0;
	wconn->wh = 0;
	wconn->timer = 0;
	wconn->connected = 0;
	wconn->conv_id = 1;
	wconn->gsc = 0;
	wconn->sslrh = 0;
	wconn->sslwh = 0;

	const char *username = purple_account_get_username(acct);
	const char *password = purple_account_get_password(acct);
	const char *nickname = purple_account_get_string(acct, "nick", "");

	wconn->waAPI = new WhatsappConnection(username, password, nickname);
	purple_connection_set_protocol_data(gc, wconn);

	const char *hostname = purple_account_get_string(acct, "server", "");
	int port = purple_account_get_int(acct, "port", WHATSAPP_DEFAULT_PORT);

	char hn[256];
	if (strlen(hostname) == 0) {
		sprintf(hn, "e%d.whatsapp.net", rand() % 9 + 1);
		hostname = hn;
	}

	if (purple_proxy_connect(gc, acct, hostname, port, waprpl_connect_cb, gc) == NULL) {
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Unable to connect");
	}

	static int sig_con = 0;
	if (!sig_con) {
		sig_con = 1;
		purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", _whatsapp_protocol, PURPLE_CALLBACK(waprpl_blist_node_removed), NULL);
		purple_signal_connect(purple_blist_get_handle(), "blist-node-added", _whatsapp_protocol, PURPLE_CALLBACK(waprpl_blist_node_added), NULL);
	}
}

static void waprpl_close(PurpleConnection * gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	if (wconn->rh)
		purple_input_remove(wconn->rh);
	if (wconn->wh)
		purple_input_remove(wconn->wh);
	if (wconn->timer)
		purple_timeout_remove(wconn->timer);

	if (wconn->fd >= 0)
		sys_close(wconn->fd);

	if (wconn->waAPI)
		delete wconn->waAPI;
	wconn->waAPI = NULL;

	g_free(wconn);
	purple_connection_set_protocol_data(gc, 0);
}

static int waprpl_send_im(PurpleConnection * gc, const char *who, const char *message, PurpleMessageFlags flags)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	char *plain;

	purple_markup_html_to_xhtml(message, NULL, &plain);

	std::string msgid = wconn->waAPI->getMessageId();
	purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-sending-message", gc, msgid.c_str(), who, message);

	wconn->waAPI->sendChat(msgid, who, plain);
	g_free(plain);

	waprpl_check_output(gc);

	return 1;
}

static int waprpl_send_chat(PurpleConnection * gc, int id, const char *message, PurpleMessageFlags flags)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	PurpleAccount *account = purple_connection_get_account(gc);
	PurpleConversation *convo = purple_find_chat(gc, id);
	PurpleChat *ch = blist_find_chat_by_convo(gc, id);
	GHashTable *hasht = purple_chat_get_components(ch);
	char *chat_id = (char*)g_hash_table_lookup(hasht, "id");
	char *plain;

	if (!chat_id) {
		purple_notify_error(gc, "Group not found", "Group not found",
			"Could not send a message to this group. It probably means that you don't belong to this group");
		return 0;
	}

	purple_markup_html_to_xhtml(message, NULL, &plain);

	std::string msgid = wconn->waAPI->getMessageId();
	purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-sending-message", gc, msgid.c_str(), chat_id, message);

	wconn->waAPI->sendGroupChat(msgid, chat_id, plain);
	g_free(plain);

	waprpl_check_output(gc);

	const char *me = purple_account_get_string(account, "nick", "");
	purple_conv_chat_write(PURPLE_CONV_CHAT(convo), me, message, PURPLE_MESSAGE_SEND, time(NULL));

	return 1;
}

static void waprpl_add_buddy(PurpleConnection * gc, PurpleBuddy * buddy, PurpleGroup * group)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	const char *name = purple_buddy_get_name(buddy);

	wconn->waAPI->addContacts({name});
	wconn->waAPI->contactsUpdate();

	waprpl_check_output(gc);
}

static void waprpl_add_buddies(PurpleConnection * gc, GList * buddies, GList * groups)
{
	GList *buddy = buddies;
	GList *group = groups;

	while (buddy && group) {
		waprpl_add_buddy(gc, (PurpleBuddy *) buddy->data, (PurpleGroup *) group->data);
		buddy = g_list_next(buddy);
		group = g_list_next(group);
	}
}

static void waprpl_remove_buddy(PurpleConnection * gc, PurpleBuddy * buddy, PurpleGroup * group)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	const char *name = purple_buddy_get_name(buddy);

	//waAPI_delcontact(wconn->waAPI, name);

	waprpl_check_output(gc);
}

static void waprpl_remove_buddies(PurpleConnection * gc, GList * buddies, GList * groups)
{
	GList *buddy = buddies;
	GList *group = groups;

	while (buddy && group) {
		waprpl_remove_buddy(gc, (PurpleBuddy *) buddy->data, (PurpleGroup *) group->data);
		buddy = g_list_next(buddy);
		group = g_list_next(group);
	}
}

static void waprpl_convo_closed(PurpleConnection * gc, const char *who)
{
	/* TODO */
}

static void waprpl_add_deny(PurpleConnection * gc, const char *name)
{
	/* TODO Do we need to implement deny? Or purple provides it? */
}

static void waprpl_rem_deny(PurpleConnection * gc, const char *name)
{
	/* TODO Do we need to implement deny? Or purple provides it? */
}

static unsigned int waprpl_send_typing(PurpleConnection * gc, const char *who, PurpleTypingState typing)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	int status = 0;
	if (typing == PURPLE_TYPING)
		status = 1;

	purple_debug_info(WHATSAPP_ID, "purple: %s typing status: %d\n", who, typing);

	wconn->waAPI->notifyTyping(who, status);
	waprpl_check_output(gc);

	return 1;
}

static void waprpl_set_buddy_icon(PurpleConnection * gc, PurpleStoredImage * img)
{
	/* Send the picture the user has selected! */
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	size_t size = purple_imgstore_get_size(img);
	const void *data = purple_imgstore_get_data(img);

	if (data) {
		// First of all make the picture a square
		char * sqbuffer; int sqsize;
		imgProfile((unsigned char*)data, size, (void**)&sqbuffer, &sqsize, 640);

		char * pbuffer; int osize;
		imgProfile((unsigned char*)data, size, (void**)&pbuffer, &osize, 96);

		wconn->waAPI->send_avatar(std::string(sqbuffer, sqsize), std::string(pbuffer, osize));

		free(sqbuffer); free(pbuffer);
	}
	else {
		wconn->waAPI->send_avatar("","");
	}

	waprpl_check_output(gc);
}

static gboolean waprpl_can_receive_file(PurpleConnection * gc, const char *who)
{
	return TRUE;
}

static gboolean waprpl_offline_message(const PurpleBuddy * buddy)
{
	return FALSE;
}

static GList *waprpl_status_types(PurpleAccount * acct)
{
	GList *types = NULL;
	PurpleStatusType *type;

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE, "available", NULL, TRUE, TRUE, FALSE, "message", "Message", purple_value_new(PURPLE_TYPE_STRING), NULL);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_UNAVAILABLE, "available-noread", NULL, TRUE, TRUE, FALSE, "message", "Message", purple_value_new(PURPLE_TYPE_STRING), NULL);
	types = g_list_prepend(types, type);

	type = purple_status_type_new_with_attrs(PURPLE_STATUS_AWAY, "unavailable", NULL, TRUE, TRUE, FALSE, "message", "Message", purple_value_new(PURPLE_TYPE_STRING), NULL);
	types = g_list_prepend(types, type);

	type = purple_status_type_new(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE);
	types = g_list_append(types, type);

	return g_list_reverse(types);
}

static void waprpl_set_status(PurpleAccount * acct, PurpleStatus * status)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(purple_account_get_connection(acct));
	const char *sid = purple_status_get_id(status);
	const char *mid = purple_status_get_attr_string(status, "message");
	if (mid == 0)
		mid = "";

	wconn->waAPI->setMyPresence(sid, mid);
	waprpl_check_output(purple_account_get_connection(acct));
}

static void waprpl_get_info(PurpleConnection * gc, const char *username)
{
	PurpleNotifyUserInfo *info = purple_notify_user_info_new();
	purple_debug_info(WHATSAPP_ID, "Fetching %s's user info for %s\n", username, gc->account->username);

	/* Get user status */
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	std::string status_string = wconn->waAPI->getUserStatusString(username);
	/* Get user picture (big one) */
	char *profile_image = g_strdup("");
	std::string icon;
	bool res = wconn->waAPI->query_avatar(username, icon);
	if (res) {
		int iid = purple_imgstore_add_with_id(g_memdup(icon.c_str(), icon.size()), icon.size(), NULL);
		profile_image = g_strdup_printf("<img id=\"%u\">", iid);
	}

	purple_notify_user_info_add_pair(info, "Status", status_string.c_str());
	purple_notify_user_info_add_pair(info, "Profile image", profile_image);

	if (res)
		g_free(profile_image);

	purple_notify_userinfo(gc, username, info, NULL, NULL);
	
	waprpl_check_output(gc);
}

static void waprpl_group_buddy(PurpleConnection * gc, const char *who, const char *old_group, const char *new_group)
{
	/* TODO implement local groups */
}

static void waprpl_rename_group(PurpleConnection * gc, const char *old_name, PurpleGroup * group, GList * moved_buddies)
{
	/* TODO implement local groups */
}

static void waprpl_insert_contacts(PurpleConnection * gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	GSList *buddies = purple_find_buddies(purple_connection_get_account(gc), NULL);
	GSList *l;

	for (l = buddies; l; l = l->next) {
		PurpleBuddy *b = (PurpleBuddy*)l->data;
		const char *name = purple_buddy_get_name(b);

		wconn->waAPI->addContacts({name});
	}

	wconn->waAPI->contactsUpdate();
	waprpl_check_output(gc);
	g_slist_free(buddies);
}

/* WA group support as chats */
static GList *waprpl_chat_join_info(PurpleConnection * gc)
{
	struct proto_chat_entry *pce;

	pce = g_new0(struct proto_chat_entry, 1);
	pce->label = "_Subject:";
	pce->identifier = "subject";
	pce->required = TRUE;
	return g_list_append(NULL, pce);
}

static GHashTable *waprpl_chat_info_defaults(PurpleConnection * gc, const char *chat_name)
{
	GHashTable *defaults = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	if (chat_name != NULL)
		g_hash_table_insert(defaults, g_strdup("subject"), g_strdup(chat_name));

	return defaults;
}

static void waprpl_chat_join(PurpleConnection * gc, GHashTable * data)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	const char *groupname = (char*)g_hash_table_lookup(data, "subject");
	char *id = (char*)g_hash_table_lookup(data, "id");

	if (!id) {
		gchar *tmp = g_strdup_printf("Joining %s requires an invitation.", groupname);
		purple_notify_error(gc, "Invitation only", "Invitation only", tmp);
		g_free(tmp);
		return;
	}

	int prplid = chatid_to_convo(id);
	purple_debug_info(WHATSAPP_ID, "joining group %s\n", groupname);

	if (!purple_find_chat(gc, prplid)) {
		std::string subject = "Unknown", owner = "00000", part, admins = "00000";
		std::map < std::string, Group > glist = wconn->waAPI->getGroups();
		if (glist.find(id) != glist.end()) {
			subject = glist.at(id).subject;
			owner   = glist.at(id).owner;
			admins  = glist.at(id).getAdminList();
			part    = glist.at(id).getParticipantsList();
		}

		/* Notify chat add */
		PurpleConversation *conv = serv_got_joined_chat(gc, prplid, groupname);

		/* Add people in the chat */
		purple_debug_info(WHATSAPP_ID, "group info ID(%s) SUBJECT(%s) OWNER(%s)\n", id, subject.c_str(), owner.c_str());
		conv_add_participants(conv, part.c_str(), owner.c_str(), admins.c_str());
	}
}

static void waprpl_chat_invite(PurpleConnection * gc, int id, const char *message, const char *name)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	PurpleAccount *acct = purple_connection_get_account(gc);
	PurpleConversation *convo = purple_find_chat(gc, id);
	PurpleChat *ch = blist_find_chat_by_convo(gc, id);
	GHashTable *hasht = purple_chat_get_components(ch);
	char *chat_id = (char*)g_hash_table_lookup(hasht, "id");
	char *admins = (char*)g_hash_table_lookup(hasht, "admins");
	const char * me = purple_account_get_username(acct);

	int imadmin = 0;
	gchar **adminl = g_strsplit(admins, ",", 0);
	gchar **p;
	for (p = adminl; *p; p++)
		if (!strcmp(me, *p))
			imadmin = 1;
	g_strfreev(adminl);
	
	if (!imadmin) {
		// Show error
		purple_notify_error(gc, "Admin privileges required", "Admin privileges required",
			"You are not an admin of this group, you cannot add more participants");
		return;
	}

	if (strstr(name, "@" WHATSAPP_SERVER) == 0)
		name = g_strdup_printf("%s@" WHATSAPP_SERVER, name);
	wconn->waAPI->manageParticipant(chat_id, name, "add");

	purple_conv_chat_add_user(purple_conversation_get_chat_data(convo), name, "", PURPLE_CBFLAGS_NONE, FALSE);

	waprpl_check_output(gc);
}

static char *waprpl_get_chat_name(GHashTable * data)
{
	return g_strdup((char*)g_hash_table_lookup(data, "subject"));
}

void waprpl_ssl_output_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		int datatosend = wconn->waAPI->sendSSLCallback(tempbuff, sizeof(tempbuff));
		purple_debug_info(WHATSAPP_ID, "Output data to send %d\n", datatosend);

		if (datatosend == 0)
			break;

		ret = purple_ssl_write(wconn->gsc, tempbuff, datatosend);

		if (ret > 0) {
			wconn->waAPI->sentSSLCallback(ret);
		} else if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
			/* Check later */
		} else {
			waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);
		}
	} while (ret > 0);

	/* Check if we need to callback again or not */
	waprpl_check_ssl_output(gc);
	waprpl_check_output(gc);
}

/* Try to read some data and push it to the WhatsApp API */
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	/* End point closed the connection */
	if (!g_list_find(purple_connections_get_all(), gc)) {
		waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);
		return;
	}

	char tempbuff[16*1024];
	int ret;
	do {
		ret = purple_ssl_read(wconn->gsc, tempbuff, sizeof(tempbuff));
		purple_debug_info(WHATSAPP_ID, "Input data read %d %d\n", ret, errno);

		if (ret > 0) {
			wconn->waAPI->receiveSSLCallback(tempbuff, ret);
		} else if (ret < 0 && errno == EAGAIN)
			break;
		else if (ret < 0) {
			waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);
			break;
		} else {
			waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);
		}
	} while (ret > 0);

	waprpl_check_ssl_output(gc);	/* The input data may generate responses! */
	waprpl_check_output(gc);
}

static void waprpl_check_complete_uploads(PurpleConnection * gc) {
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	
	GList *xfers = purple_xfers_get_all();
	while (xfers) {
		PurpleXfer *xfer = (PurpleXfer*)xfers->data;
		wa_file_transfer *xinfo = (wa_file_transfer *) xfer->data;
		if (xinfo->upload) {
			if (!xinfo->done && xinfo->started && wconn->waAPI->uploadComplete(xinfo->ref_id)) {
				purple_debug_info(WHATSAPP_ID, "Upload complete\n");
				purple_xfer_set_completed(xfer, TRUE);
				xinfo->done = 1;
			}
		}
		xfers = g_list_next(xfers);
	}
}

/* Checks if the WA protocol has data to output and schedules a write handler */
void waprpl_check_ssl_output(PurpleConnection * gc)
{
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	if (wconn->sslfd >= 0) {

		if (wconn->waAPI->hasSSLDataToSend()) {
			/* Need to watch for output data (if we are not doing it already) */
			if (wconn->sslwh == 0)
				wconn->sslwh = purple_input_add(wconn->sslfd, PURPLE_INPUT_WRITE, waprpl_ssl_output_cb, gc);
		} else if (wconn->waAPI->closeSSLConnection()) {
			waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);	/* Finished the connection! */
		} else {
			if (wconn->sslwh != 0)
				purple_input_remove(wconn->sslwh);

			wconn->sslwh = 0;
		}

		/* Update transfer status */
		int rid, bytes_sent;
		if (wconn->waAPI->uploadProgress(rid, bytes_sent)) {
			GList *xfers = purple_xfers_get_all();
			while (xfers) {
				PurpleXfer *xfer = (PurpleXfer*)xfers->data;
				wa_file_transfer *xinfo = (wa_file_transfer *) xfer->data;
				if (xinfo->upload && xinfo->ref_id == rid) {
					purple_debug_info(WHATSAPP_ID, "Upload progress %d bytes done\n", bytes_sent);
					purple_xfer_set_bytes_sent(xfer, bytes_sent);
					purple_xfer_update_progress(xfer);
					break;
				}
				xfers = g_list_next(xfers);
			}
		}

	}
	
	// Check uploads to mark them as done :)
	waprpl_check_complete_uploads(gc);
}

static void waprpl_ssl_connected_cb(gpointer data, PurpleSslConnection * gsc, PurpleInputCondition cond)
{
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	if (!wconn) return; // The account has disconnected 

	purple_debug_info(WHATSAPP_ID, "SSL connection stablished\n");

	wconn->sslfd = gsc->fd;
	wconn->sslrh = purple_input_add(wconn->sslfd, PURPLE_INPUT_READ, waprpl_ssl_input_cb, gc);
	waprpl_check_ssl_output(gc);
}

void waprpl_ssl_cerr_cb(PurpleSslConnection * gsc, PurpleSslErrorType error, gpointer data)
{
	/* Do not use gsc, may be null */
	PurpleConnection *gc = (PurpleConnection*)data;
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);
	if (!wconn)
		return;

	if (wconn->sslwh != 0)
		purple_input_remove(wconn->sslwh);
	if (wconn->sslrh != 0)
		purple_input_remove(wconn->sslrh);

	wconn->waAPI->SSLCloseCallback();

	/* The connection is closed and freed automatically. */
	wconn->gsc = NULL;

	wconn->sslfd = -1;
	wconn->sslrh = 0;
	wconn->sslwh = 0;
}

void check_ssl_requests(PurpleAccount * acct)
{
	PurpleConnection *gc = purple_account_get_connection(acct);
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	std::string host;
	int port;
	if (wconn->gsc == 0 && wconn->waAPI->hasSSLConnection(host, port)) {
		purple_debug_info(WHATSAPP_ID, "Establishing SSL connection to %s:%d\n", host.c_str(), port);

		PurpleSslConnection *sslc = purple_ssl_connect(acct, host.c_str(), port, waprpl_ssl_connected_cb, waprpl_ssl_cerr_cb, gc);
		if (sslc == 0) {
			waprpl_ssl_cerr_cb(0, PURPLE_SSL_CONNECT_FAILED, gc);
		} else {
			/* The Fd are not available yet, wait for connected callback */
			wconn->gsc = sslc;
		}
	}
}

void waprpl_xfer_init_sendimg(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "File upload xfer init...\n");
	wa_file_transfer *xinfo = (wa_file_transfer *) xfer->data;
	whatsapp_connection *wconn = (whatsapp_connection*)xinfo->wconn;

	size_t fs = purple_xfer_get_size(xfer);
	const char *fn = purple_xfer_get_filename(xfer);
	const char *fp = purple_xfer_get_local_filename(xfer);

	wa_file_transfer *xfer_info = (wa_file_transfer *) xfer->data;
	purple_xfer_set_size(xfer, fs);

	std::string msgid = wconn->waAPI->getMessageId();

	xfer_info->ref_id = wconn->waAPI->sendImage(msgid, xinfo->to, 100, 100, fs, fp);
	xfer_info->started = 1;
	purple_debug_info(WHATSAPP_ID, "Transfer file %s at %s with size %zu (given ref %d)\n", fn, fp, fs, xfer_info->ref_id);

	waprpl_check_output(xinfo->gc);
}

void http_download_cb(PurpleUtilFetchUrlData *url_data, gpointer user_data, const gchar *url_text,
	gsize len, const gchar *error_message)
{
	if (len > 0) {
		purple_debug_info(WHATSAPP_ID, "Got some HTTP data! %d\n", len);
		PurpleXfer * xfer = (PurpleXfer *)user_data;
		wa_file_transfer *xinfo = (wa_file_transfer *) xfer->data;
		whatsapp_connection *wconn = (whatsapp_connection*)xinfo->wconn;

		std::string encoded_img(url_text, len);

		purple_xfer_set_size(xfer, len);
		purple_xfer_set_bytes_sent(xfer, len);
		purple_xfer_update_progress(xfer);

		std::string decoded_img = wconn->waAPI->decodeImage(encoded_img, xinfo->iv, xinfo->aeskey);

		unsigned char* buffer = (unsigned char*)decoded_img.data();
		unsigned size = decoded_img.size();

		//PurpleXferUiOps *ui_ops = purple_xfer_get_ui_ops(xfer);
		//purple_xfer_set_bytes_sent(xfer, purple_xfer_get_bytes_sent(xfer) + 
		//	(ui_ops && ui_ops->ui_write ? ui_ops->ui_write(xfer, buffer, size) : fwrite(buffer, 1, size, xfer->dest_fp)));


		int imgid = purple_imgstore_add_with_id(g_memdup(buffer, size), size, NULL);
		char *msg = g_strdup_printf("<img id=\"%u\">", imgid);
		conv_add_message(xinfo->gc, xinfo->to, msg, xinfo->to, 0);
		g_free(msg);

		purple_xfer_set_completed(xfer, TRUE);
	}
	else {
		purple_debug_info(WHATSAPP_ID, "Got some trouble downloading the data...!\n");
	}
}

void waprpl_xfer_init_receiveimg(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "File download xfer init...\n");
	wa_file_transfer *xinfo = (wa_file_transfer *) xfer->data;
	whatsapp_connection *wconn = (whatsapp_connection*)xinfo->wconn;

	//purple_xfer_start(xfer, -1, NULL, 0);
	xinfo->started = 1;

	PurpleUtilFetchUrlData* url_fetch = purple_util_fetch_url_request_len_with_account(
		purple_connection_get_account(xinfo->gc), xinfo->url.c_str(), TRUE, NULL, TRUE, NULL, FALSE, -1,
		http_download_cb, xfer);

	waprpl_check_output(xinfo->gc);
}

void waprpl_xfer_start(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "Starting file tranfer...\n");
}

void waprpl_xfer_end(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "Ended file tranfer!\n");
}

void waprpl_xfer_cancel_send(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "File tranfer cancel send!!!\n");
	/* TODO: Add cancel call, should be pretty easy */
}

/* Send file (used for sending images?) */
static PurpleXfer *waprpl_new_xfer_upload(PurpleConnection * gc, const char *who)
{
	purple_debug_info(WHATSAPP_ID, "New file xfer\n");
	PurpleXfer *xfer = purple_xfer_new(gc->account, PURPLE_XFER_SEND, who);
	g_return_val_if_fail(xfer != NULL, NULL);
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	wa_file_transfer *xfer_info = new wa_file_transfer();
	xfer_info->upload = true;
	xfer_info->to = g_strdup(who);
	xfer->data = xfer_info;
	xfer_info->wconn = wconn;
	xfer_info->gc = gc;
	xfer_info->done = 0;
	xfer_info->started = 0;

	purple_xfer_set_init_fnc(xfer, waprpl_xfer_init_sendimg);
	purple_xfer_set_start_fnc(xfer, waprpl_xfer_start);
	purple_xfer_set_end_fnc(xfer, waprpl_xfer_end);
	purple_xfer_set_cancel_send_fnc(xfer, waprpl_xfer_cancel_send);

	return xfer;
}

static void waprpl_send_file(PurpleConnection * gc, const char *who, const char *file)
{
	purple_debug_info(WHATSAPP_ID, "Send file called\n");
	PurpleXfer *xfer = waprpl_new_xfer_upload(gc, who);

	if (file) {
		purple_xfer_request_accepted(xfer, file);
		purple_debug_info(WHATSAPP_ID, "Accepted transfer of file %s\n", file);
	} else
		purple_xfer_request(xfer);
}

static PurpleXfer *waprpl_new_xfer_download(PurpleConnection * gc, const char *who, std::string url, std::string iv, std::string k)
{
	purple_debug_info(WHATSAPP_ID, "New file xfer (download)\n");
	PurpleXfer *xfer = purple_xfer_new(gc->account, PURPLE_XFER_RECEIVE, who);
	g_return_val_if_fail(xfer != NULL, NULL);
	whatsapp_connection *wconn = (whatsapp_connection*)purple_connection_get_protocol_data(gc);

	wa_file_transfer *xfer_info = new wa_file_transfer();
	xfer_info->upload = false;
	xfer_info->to = g_strdup(who);
	xfer->data = xfer_info;
	xfer_info->wconn = wconn;
	xfer_info->gc = gc;
	xfer_info->done = 0;
	xfer_info->started = 0;
	xfer_info->url = url;
	xfer_info->iv = iv;
	xfer_info->aeskey = k;

	purple_xfer_set_init_fnc(xfer, waprpl_xfer_init_receiveimg);
	purple_xfer_set_start_fnc(xfer, waprpl_xfer_start);
	purple_xfer_set_end_fnc(xfer, waprpl_xfer_end);
	purple_xfer_set_cancel_send_fnc(xfer, waprpl_xfer_cancel_send);

	return xfer;
}

void waprpl_image_download_offer(PurpleConnection * gc, std::string from, std::string url, bool ciphered,
	std::string iv, std::string aeskey)
{
	purple_debug_info(WHATSAPP_ID, "Received a file transfer request!\n");
	PurpleXfer *xfer = waprpl_new_xfer_download(gc, from.c_str(), url, iv, aeskey);

	if (xfer)
		purple_xfer_request(xfer);
}


extern "C" {

static PurplePluginProtocolInfo prpl_info = {
	(PurpleProtocolOptions)0,			/* options */
	NULL,			/* user_splits, initialized in waprpl_init() */
	NULL,			/* protocol_options, initialized in waprpl_init() */
	{			/* icon_spec, a PurpleBuddyIconSpec */
		"png,gif,bmp,tiff,jpg",			/* format */
		1,			/* min_width */
		1,			/* min_height */
		4096,			/* max_width */
		4096,			/* max_height */
		8*1024*1024,	/* max_filesize */
		PURPLE_ICON_SCALE_SEND,	/* scale_rules */
	},
	waprpl_list_icon,	/* list_icon */
	NULL,			/* list_emblem */
	waprpl_status_text,	/* status_text */
	waprpl_tooltip_text,	/* tooltip_text */
	waprpl_status_types,	/* status_types */
	NULL,			/* blist_node_menu */
	waprpl_chat_join_info,	/* chat_info */
	waprpl_chat_info_defaults,	/* chat_info_defaults */
	waprpl_login,		/* login */
	waprpl_close,		/* close */
	waprpl_send_im,		/* send_im */
	NULL,			/* set_info */
	waprpl_send_typing,	/* send_typing */
	waprpl_get_info,	/* get_info */
	waprpl_set_status,	/* set_status */
	NULL,			/* set_idle */
	NULL,			/* change_passwd */
	waprpl_add_buddy,	/* add_buddy */
	waprpl_add_buddies,	/* add_buddies */
	waprpl_remove_buddy,	/* remove_buddy */
	waprpl_remove_buddies,	/* remove_buddies */
	NULL,			/* add_permit */
	waprpl_add_deny,	/* add_deny */
	NULL,			/* rem_permit */
	waprpl_rem_deny,	/* rem_deny */
	NULL,			/* set_permit_deny */
	waprpl_chat_join,	/* join_chat */
	NULL,			/* reject_chat */
	waprpl_get_chat_name,	/* get_chat_name */
	waprpl_chat_invite,	/* chat_invite */
	NULL,			/* chat_leave */
	NULL,			/* chat_whisper */
	waprpl_send_chat,	/* chat_send */
	NULL,			/* keepalive */
	NULL,			/* register_user */
	NULL,			/* get_cb_info */
	NULL,			/* get_cb_away */
	NULL,			/* alias_buddy */
	waprpl_group_buddy,	/* group_buddy */
	waprpl_rename_group,	/* rename_group */
	NULL,			/* buddy_free */
	waprpl_convo_closed,	/* convo_closed */
	purple_normalize_nocase,	/* normalize */
	waprpl_set_buddy_icon,	/* set_buddy_icon */
	NULL,			/* remove_group */
	NULL,			/* get_cb_real_name */
	NULL,			/* set_chat_topic */
	NULL,			/* find_blist_chat */
	NULL,			/* roomlist_get_list */
	NULL,			/* roomlist_cancel */
	NULL,			/* roomlist_expand_category */
	waprpl_can_receive_file,	/* can_receive_file */
	waprpl_send_file,	/* send_file */
	NULL,			/* new_xfer */
	waprpl_offline_message,	/* offline_message */
	NULL,			/* whiteboard_prpl_ops */
	NULL,			/* send_raw */
	NULL,			/* roomlist_room_serialize */
	NULL,			/* unregister_user */
	NULL,			/* send_attention */
	NULL,			/* get_attention_types */
	sizeof(PurplePluginProtocolInfo),	/* struct_size */
	NULL,			/* get_account_text_table */
	NULL,			/* initiate_media */
	NULL,			/* get_media_caps */
	NULL,			/* get_moods */
	NULL,			/* set_public_alias */
	NULL,			/* get_public_alias */
	NULL,			/* add_buddy_with_invite */
	NULL			/* add_buddies_with_invite */
};

static void waprpl_init(PurplePlugin * plugin)
{
	PurpleAccountOption *option;

	prpl_info.user_splits = NULL;

	option = purple_account_option_string_new("Server", "server", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new("Port", "port", WHATSAPP_DEFAULT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new("Nickname", "nick", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new("Resource", "resource", default_resource);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_bool_new("Send ciphered messages (when possible)", "send_ciphered", TRUE);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_whatsapp_protocol = plugin;

	// Some signals which can be caught by plugins
	purple_signal_register(plugin, "whatsapp-sending-message",
			purple_marshal_VOID__POINTER_POINTER_POINTER_POINTER,
			NULL, 4,
			purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
			purple_value_new(PURPLE_TYPE_STRING), /* id */
			purple_value_new(PURPLE_TYPE_STRING), /* who */
			purple_value_new(PURPLE_TYPE_STRING)  /* message */
	);
	purple_signal_register(plugin, "whatsapp-message-received",
			purple_marshal_VOID__POINTER_POINTER_UINT,
			NULL, 3,
			purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
			purple_value_new(PURPLE_TYPE_STRING),  /* id */
			purple_value_new(PURPLE_TYPE_INT)      /* reception-types */
	);
	purple_signal_register(plugin, "whatsapp-message-error",
			purple_marshal_VOID__POINTER_POINTER_UINT,
			NULL, 2,
			purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
			purple_value_new(PURPLE_TYPE_STRING)  /* id */
	);
}

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,	/* magic */
	PURPLE_MAJOR_VERSION,	/* major_version */
	PURPLE_MINOR_VERSION,	/* minor_version */
	PURPLE_PLUGIN_PROTOCOL,	/* type */
	NULL,			/* ui_requirement */
	0,			/* flags */
	NULL,			/* dependencies */
	PURPLE_PRIORITY_DEFAULT,	/* priority */
	"prpl-whatsapp",		/* id */
	"WhatsApp",		/* name */
	"0.8.6",			/* version */
	"WhatsApp protocol for libpurple",	/* summary */
	"WhatsApp protocol for libpurple",	/* description */
	"David Guillen Fandos (david@davidgf.net)",	/* author */
	"http://davidgf.net",	/* homepage */
	NULL,			/* load */
	NULL,			/* unload */
	NULL,			/* destroy */
	NULL,			/* ui_info */
	&prpl_info,		/* extra_info */
	NULL,			/* prefs_info */
	waprpl_actions,		/* actions */
	NULL,			/* padding... */
	NULL,
	NULL,
	NULL,
};

PURPLE_INIT_PLUGIN(whatsapp, waprpl_init, info);

}
