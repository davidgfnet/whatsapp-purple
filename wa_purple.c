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
#include "version.h"

#include "wa_api.h"

#ifdef _WIN32
#define sys_read  wpurple_read
#define sys_write wpurple_write
#define sys_close wpurple_close
#else
#include <unistd.h>
#define sys_read  read
#define sys_write write
#define sys_close close
#endif

const char default_resource[] = "WP7-2.11.596-443";

#define WHATSAPP_ID "whatsapp"
static PurplePlugin *_whatsapp_protocol = NULL;

#define WHATSAPP_STATUS_ONLINE   "online"
#define WHATSAPP_STATUS_AWAY     "away"
#define WHATSAPP_STATUS_OFFLINE  "offline"

#define WHATSAPP_DEFAULT_SERVER "c2.whatsapp.net"
#define WHATSAPP_DEFAULT_PORT   443

typedef struct {
	unsigned int file_size;
	char *to;
	void *wconn;
	PurpleConnection *gc;
	int ref_id;
	int done, started;
} wa_file_upload;

typedef struct {
	PurpleAccount *account;
	int fd;			/* File descriptor of the socket */
	guint rh, wh;		/* Read/write handlers */
	int connected;		/* Connection status */
	void *waAPI;		/* Pointer to the C++ class which actually implements the protocol */
	int conv_id;		/* Combo id counter */
	/* HTTPS interface for status query */
	guint sslrh, sslwh;	/* Read/write handlers */
	int sslfd;
	PurpleSslConnection *gsc;	/* SSL handler */
} whatsapp_connection;

static void waprpl_check_output(PurpleConnection * gc);
static void waprpl_process_incoming_events(PurpleConnection * gc);
static void waprpl_insert_contacts(PurpleConnection * gc);
char *last_seen_text(unsigned long long t);
static void waprpl_chat_join(PurpleConnection * gc, GHashTable * data);
void check_ssl_requests(PurpleAccount * acct);
void waprpl_ssl_cerr_cb(PurpleSslConnection * gsc, PurpleSslErrorType error, gpointer data);
void waprpl_check_ssl_output(PurpleConnection * gc);
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond);
static void waprpl_set_status(PurpleAccount * acct, PurpleStatus * status);
static void waprpl_check_complete_uploads(PurpleConnection * gc);

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
	int st = waAPI_getuserstatus(wconn->waAPI, purple_buddy_get_name(buddy));
	if (st < 0)
		status = "Unknown";
	else if (st == 0)
		status = "Unavailable";
	else
		status = "Available";
	unsigned long long lseen = waAPI_getlastseen(wconn->waAPI, purple_buddy_get_name(buddy));
	char * statusmsg = waAPI_getuserstatusstring(wconn->waAPI, purple_buddy_get_name(buddy));
	purple_notify_user_info_add_pair_plaintext(info, "Status", status);
	purple_notify_user_info_add_pair_plaintext(info, "Last seen on WhatsApp", purple_str_seconds_to_string(lseen));
	purple_notify_user_info_add_pair_plaintext(info, "Status message", statusmsg);
}

static char *waprpl_status_text(PurpleBuddy * buddy)
{
	char *statusmsg;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
	if (!wconn)
		return 0;

	statusmsg = waAPI_getuserstatusstring(wconn->waAPI, purple_buddy_get_name(buddy));
	if (!statusmsg || strlen(statusmsg) == 0)
		return NULL;
	return statusmsg;
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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	if (!wconn)
		return;

	unsigned long long creation, freeexpires;
	char *status;
	waAPI_accountinfo(wconn->waAPI, &creation, &freeexpires, &status);

	time_t creationtime = creation;
	time_t freeexpirestime = freeexpires;
	char *cr = g_strdup(asctime(localtime(&creationtime)));
	char *ex = g_strdup(asctime(localtime(&freeexpirestime)));
	char *text = g_strdup_printf("Account status: %s<br />Created on: %s<br />Free expires on: %s\n", status, cr, ex);

	purple_notify_formatted(gc, "Account information", "Account information", "", text, NULL, NULL);

	g_free(text);
	g_free(ex);
	g_free(cr);
}

static GList *waprpl_actions(PurplePlugin * plugin, gpointer context)
{
	PurplePluginAction *act;

	act = purple_plugin_action_new("Show account information ...", waprpl_show_accountinfo);
	return g_list_append(NULL, act);
}

static int isgroup(const char *user)
{
	return (strchr(user, '-') != NULL);
}

static void waprpl_blist_node_removed(PurpleBlistNode * node)
{
	if (!PURPLE_BLIST_NODE_IS_CHAT(node))
		return;

	PurpleChat *ch = PURPLE_CHAT(node);
	PurpleConnection *gc = purple_account_get_connection(purple_chat_get_account(ch));
	if (purple_connection_get_prpl(gc) != _whatsapp_protocol)
		return;

	char *gid = g_hash_table_lookup(purple_chat_get_components(ch), "id");
	if (gid == 0)
		return;		/* Group is not created yet... */
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	waAPI_deletegroup(wconn->waAPI, gid);
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

	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	GHashTable *hasht = purple_chat_get_components(ch);
	const char *groupname = g_hash_table_lookup(hasht, "subject");
	const char *gid = g_hash_table_lookup(hasht, "id");
	if (gid != 0)
		return;		/* Already created */
	purple_debug_info(WHATSAPP_ID, "Creating group %s\n", groupname);

	waAPI_creategroup(wconn->waAPI, groupname);
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
	return !strcmp(g_hash_table_lookup(hasht, "id"), *((char **)data));
}

static int hasht_cmp_convo(GHashTable *hasht, void *data)
{
	return (chatid_to_convo(g_hash_table_lookup(hasht, "id")) == *((int *)data));
}

static PurpleChat *blist_find_chat_by_id(PurpleConnection *gc, const char *id)
{
	return blist_find_chat_by_hasht_cond(gc, hasht_cmp_id, &id);
}

static PurpleChat *blist_find_chat_by_convo(PurpleConnection *gc, int convo)
{
	return blist_find_chat_by_hasht_cond(gc, hasht_cmp_convo, &convo);
}


static void conv_add_participants(PurpleConversation * conv, const char *part, const char *owner)
{
	gchar **plist = g_strsplit(part, ",", 0);
	gchar **p;

	purple_conv_chat_clear_users(purple_conversation_get_chat_data(conv));
	for (p = plist; *p; p++)
		purple_conv_chat_add_user(purple_conversation_get_chat_data(conv), *p, "", PURPLE_CBFLAGS_NONE | (!strcmp(owner, *p) ? PURPLE_CBFLAGS_FOUNDER : 0), FALSE);

	g_strfreev(plist);
}

PurpleConversation *get_open_combo(const char *who, PurpleConnection * gc)
{
	PurpleAccount *acc = purple_connection_get_account(gc);
	if (isgroup(who)) {
		/* Search fot the combo */
		PurpleChat *ch = blist_find_chat_by_id(gc, who);
		GHashTable *hasht = purple_chat_get_components(ch);
		int convo_id = chatid_to_convo(who);
		const char *groupname = g_hash_table_lookup(hasht, "subject");
		PurpleConversation *convo = purple_find_chat(gc, convo_id);
		
		/* Create a window if it's not open yet */
		if (!convo) {
			waprpl_chat_join(gc, hasht);
			convo = purple_find_chat(gc, convo_id);
		}
		else if (purple_conv_chat_has_left(PURPLE_CONV_CHAT(convo))) {
			char *subject, *owner, *part;
			whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
			if (waAPI_getgroupinfo(wconn->waAPI, (char*)who, &subject, &owner, &part)) {
				convo = serv_got_joined_chat(gc, convo_id, groupname);
				purple_debug_info(WHATSAPP_ID, "group info ID(%s) SUBJECT(%s) OWNER(%s)\n", who, subject, owner);
				conv_add_participants(convo, part, owner);
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
		serv_got_im(gc, who, msg, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_IMAGES, timestamp);
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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);
	char *who;
	int status;

	while (waAPI_querystatus(wconn->waAPI, &who, &status)) {
		if (status == 1) {
			purple_prpl_got_user_status(acc, who, "available", "message", "", NULL);
		} else {
			purple_prpl_got_user_status(acc, who, "unavailable", "message", "", NULL);
		}
	}
}

static void query_typing(PurpleConnection *gc)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	char *who;
	int status;

	while (waAPI_querytyping(wconn->waAPI, &who, &status)) {
		if (status == 1) {
			purple_debug_info(WHATSAPP_ID, "%s is typing\n", who);
			serv_got_typing(gc, who, 0, PURPLE_TYPING);
		} else {
			purple_debug_info(WHATSAPP_ID, "%s is not typing\n", who);
			serv_got_typing(gc, who, 0, PURPLE_NOT_TYPING);
			serv_got_typing_stopped(gc, who);
		}
	}
}

static void query_icon(PurpleConnection *gc)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);
	char *who, *icon, *hash;
	int len;

	while (waAPI_queryicon(wconn->waAPI, &who, &icon, &len, &hash)) {
		purple_buddy_icons_set_for_user(acc, who, g_memdup(icon, len), len, hash);
	}
}

static void waprpl_process_incoming_events(PurpleConnection * gc)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	PurpleAccount *acc = purple_connection_get_account(gc);

	switch (waAPI_loginstatus(wconn->waAPI)) {
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
	if (waAPI_getgroupsupdated(wconn->waAPI)) {

		/* Delete/update the chats that are in our list */
		PurpleBlistNode *node;

		for (node = purple_blist_get_root(); node; node = purple_blist_node_next(node, FALSE)) {
			if (!PURPLE_BLIST_NODE_IS_CHAT(node))
				continue;

			PurpleChat *ch = PURPLE_CHAT(node);
			if (purple_chat_get_account(ch) != acc)
				continue;

			GHashTable *hasht = purple_chat_get_components(ch);
			char *grid = g_hash_table_lookup(hasht, "id");
			char *glist = waAPI_getgroups(wconn->waAPI);
			gchar **gplist = g_strsplit(glist, ",", 0);

			if (str_array_find(gplist, grid) >= 0) {
				/* The group is in the system, update the fields */
				char *sub, *own;
				waAPI_getgroupinfo(wconn->waAPI, grid, &sub, &own, 0);
				g_hash_table_replace(hasht, g_strdup("subject"), g_strdup(sub));
				g_hash_table_replace(hasht, g_strdup("owner"), g_strdup(own));
				purple_blist_alias_chat(ch, g_strdup(sub));
			} else {
				/* The group was deleted */
				PurpleChat *del = (PurpleChat *) node;
				node = purple_blist_node_next(node, FALSE);
				purple_blist_remove_chat(del);
			}

			g_strfreev(gplist);
		}

		/* Add new groups */
		char *glist = waAPI_getgroups(wconn->waAPI);
		gchar **gplist = g_strsplit(glist, ",", 0);
		gchar **p;

		for (p = gplist; *p; p++) {
			gchar *gpid = *p;
			PurpleChat *ch = blist_find_chat_by_id(gc, gpid);
			if (!ch) {
				char *sub, *own;
				waAPI_getgroupinfo(wconn->waAPI, gpid, &sub, &own, 0);
				purple_debug_info(WHATSAPP_ID, "New group found %s %s\n", gpid, sub);

				GHashTable *htable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
				g_hash_table_insert(htable, g_strdup("subject"), g_strdup(sub));
				g_hash_table_insert(htable, g_strdup("id"), g_strdup(gpid));
				g_hash_table_insert(htable, g_strdup("owner"), g_strdup(own));

				ch = purple_chat_new(acc, sub, htable);
				purple_blist_add_chat(ch, NULL, NULL);
			}
			/* Now update the open conversation that may exist */
			char *id = g_hash_table_lookup(purple_chat_get_components(ch), "id");
			int prplid = chatid_to_convo(id);
			PurpleConversation *conv = purple_find_chat(gc, prplid);
			char *subject, *owner, *part;
			if (conv && waAPI_getgroupinfo(wconn->waAPI, id, &subject, &owner, &part))
				conv_add_participants(conv, part, owner);
		}

		g_strfreev(gplist);
	}

	t_message m;
	while (waAPI_querymsg(wconn->waAPI, &m)) {
		switch (m.type) {
		case 0:
			purple_debug_info(WHATSAPP_ID, "Got chat message from %s: %s\n", m.who, m.message);
			conv_add_message(gc, m.who, m.message, m.author, m.t);
			break;
		case 1: {
			purple_debug_info(WHATSAPP_ID, "Got image from %s: %s\n", m.who, m.message);
			int imgid = purple_imgstore_add_with_id(g_memdup(m.image, m.imagelen), m.imagelen, NULL);
			char *msg = g_strdup_printf("<a href=\"%s\"><img id=\"%u\"></a><br/><a href=\"%s\">%s</a>",
				m.url, imgid, m.url, m.url);
			conv_add_message(gc, m.who, msg, m.author, m.t);
			g_free(msg);
			} break;
		case 2: {
			purple_debug_info(WHATSAPP_ID, "Got geomessage from: %s Coordinates (%f %f)\n", 
				m.who, (float)m.lat, (float)m.lng);
			char * lat = dbl2str(m.lat);
			char * lng = dbl2str(m.lng);
			char *msg = g_strdup_printf("<a href=\"http://openstreetmap.org/?lat=%s&lon=%s&zoom=20\">"
				"http://openstreetmap.org/?lat=%s&lon=%s&zoom=20</a>", 
				lat, lng, lat, lng);
			conv_add_message(gc, m.who, msg, m.author, m.t);
			g_free(msg); g_free(lng); g_free(lat);
			} break;
		case 3: {
			purple_debug_info(WHATSAPP_ID, "Got chat sound from %s: %s\n", m.who, m.url);
			char *msg = g_strdup_printf("<a href=\"%s\">%s</a>", m.url, m.url);
			conv_add_message(gc, m.who, msg, m.author, m.t);
			g_free(msg);
			} break;
		case 4: {
			purple_debug_info(WHATSAPP_ID, "Got chat video from %s: %s\n", m.who, m.url);
			char *msg = g_strdup_printf("<a href=\"%s\">%s</a>", m.url, m.url);
			conv_add_message(gc, m.who, msg, m.author, m.t);
			g_free(msg);
			} break;
		default:
			purple_debug_info(WHATSAPP_ID, "Got an unrecognized message!\n");
			break;
		};
	}

	while (1) {
		int typer;
		char msgid[128];
		if (!waAPI_queryreceivedmsg(wconn->waAPI, msgid, &typer))
			break;

		purple_debug_info(WHATSAPP_ID, "Received message %s type: %d\n", msgid, typer);
		purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-message-received", gc, msgid, typer);
	}

	/* Status changes, typing notices and profile pictures. */
	query_status(gc);
	query_typing(gc);
	query_icon(gc);
}

static void waprpl_output_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		int datatosend = waAPI_sendcb(wconn->waAPI, tempbuff, sizeof(tempbuff));
		if (datatosend == 0)
			break;

		ret = sys_write(wconn->fd, tempbuff, datatosend);

		if (ret > 0) {
			waAPI_senddone(wconn->waAPI, ret);
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
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		ret = sys_read(wconn->fd, tempbuff, sizeof(tempbuff));
		if (ret > 0)
			waAPI_input(wconn->waAPI, tempbuff, ret);
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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	if (wconn->fd < 0)
		return;

	if (waAPI_hasoutdata(wconn->waAPI) > 0) {
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

static void waprpl_connect_cb(gpointer data, gint source, const gchar * error_message)
{
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	PurpleAccount *acct = purple_connection_get_account(gc);
	const char *resource = purple_account_get_string(acct, "resource", default_resource);

	if (source < 0) {
		gchar *tmp = g_strdup_printf("Unable to connect: %s", error_message);
		purple_connection_error_reason(gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
		g_free(tmp);
	} else {
		wconn->fd = source;
		waAPI_login(wconn->waAPI, resource);
		wconn->rh = purple_input_add(wconn->fd, PURPLE_INPUT_READ, waprpl_input_cb, gc);
		waprpl_check_output(gc);
	}
}

static void waprpl_login(PurpleAccount * acct)
{
	PurpleConnection *gc = purple_account_get_connection(acct);

	purple_debug_info(WHATSAPP_ID, "logging in %s\n", purple_account_get_username(acct));

	purple_connection_update_progress(gc, "Connecting", 0, 4);

	whatsapp_connection *wconn = g_new0(whatsapp_connection, 1);
	wconn->fd = -1;
	wconn->sslfd = -1;
	wconn->account = acct;
	wconn->rh = 0;
	wconn->wh = 0;
	wconn->connected = 0;
	wconn->conv_id = 1;
	wconn->gsc = 0;
	wconn->sslrh = 0;
	wconn->sslwh = 0;

	const char *username = purple_account_get_username(acct);
	const char *password = purple_account_get_password(acct);
	const char *nickname = purple_account_get_string(acct, "nick", "");

	wconn->waAPI = waAPI_create(username, password, nickname);
	purple_connection_set_protocol_data(gc, wconn);

	const char *hostname = purple_account_get_string(acct, "server", WHATSAPP_DEFAULT_SERVER);
	int port = purple_account_get_int(acct, "port", WHATSAPP_DEFAULT_PORT);

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	if (wconn->rh)
		purple_input_remove(wconn->rh);
	if (wconn->wh)
		purple_input_remove(wconn->wh);

	if (wconn->fd >= 0)
		sys_close(wconn->fd);

	if (wconn->waAPI)
		waAPI_delete(wconn->waAPI);
	wconn->waAPI = NULL;

	g_free(wconn);
	purple_connection_set_protocol_data(gc, 0);
}

static int waprpl_send_im(PurpleConnection * gc, const char *who, const char *message, PurpleMessageFlags flags)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	char *plain;

	purple_markup_html_to_xhtml(message, NULL, &plain);

	char msgid[128];
	waAPI_getmsgid(wconn->waAPI, msgid);

	purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-sending-message", gc, msgid, who, message);

	waAPI_sendim(wconn->waAPI, msgid, who, plain);
	g_free(plain);

	waprpl_check_output(gc);

	return 1;
}

static int waprpl_send_chat(PurpleConnection * gc, int id, const char *message, PurpleMessageFlags flags)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	PurpleAccount *account = purple_connection_get_account(gc);
	PurpleConversation *convo = purple_find_chat(gc, id);
	PurpleChat *ch = blist_find_chat_by_convo(gc, id);
	GHashTable *hasht = purple_chat_get_components(ch);
	char *chat_id = g_hash_table_lookup(hasht, "id");
	char *plain;

	purple_markup_html_to_xhtml(message, NULL, &plain);

	char msgid[128];
	waAPI_getmsgid(wconn->waAPI, msgid);

	purple_signal_emit(purple_connection_get_prpl(gc), "whatsapp-sending-message", gc, msgid, chat_id, message);

	waAPI_sendchat(wconn->waAPI, msgid, chat_id, plain);
	g_free(plain);

	waprpl_check_output(gc);

	const char *me = purple_account_get_string(account, "nick", "");
	purple_conv_chat_write(PURPLE_CONV_CHAT(convo), me, message, PURPLE_MESSAGE_SEND, time(NULL));

	return 1;
}

static void waprpl_add_buddy(PurpleConnection * gc, PurpleBuddy * buddy, PurpleGroup * group)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	const char *name = purple_buddy_get_name(buddy);

	waAPI_addcontact(wconn->waAPI, name);

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	const char *name = purple_buddy_get_name(buddy);

	waAPI_delcontact(wconn->waAPI, name);

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	int status = 0;
	if (typing == PURPLE_TYPING)
		status = 1;

	purple_debug_info(WHATSAPP_ID, "purple: %s typing status: %d\n", who, typing);

	waAPI_sendtyping(wconn->waAPI, who, status);
	waprpl_check_output(gc);

	return 1;
}

static void waprpl_set_buddy_icon(PurpleConnection * gc, PurpleStoredImage * img)
{
	/* Send the picture the user has selected! */
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	size_t size = purple_imgstore_get_size(img);
	const void *data = purple_imgstore_get_data(img);
	waAPI_setavatar(wconn->waAPI, data, size);

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(purple_account_get_connection(acct));
	const char *sid = purple_status_get_id(status);
	const char *mid = purple_status_get_attr_string(status, "message");
	if (mid == 0)
		mid = "";

	waAPI_setmypresence(wconn->waAPI, sid, mid);
	waprpl_check_output(purple_account_get_connection(acct));
}

static void waprpl_get_info(PurpleConnection * gc, const char *username)
{
	PurpleNotifyUserInfo *info = purple_notify_user_info_new();
	purple_debug_info(WHATSAPP_ID, "Fetching %s's user info for %s\n", username, gc->account->username);

	/* Get user status */
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	const char *status_string = waAPI_getuserstatusstring(wconn->waAPI, username);
	/* Get user picture (big one) */
	char *profile_image = "";
	char *icon;
	int len;
	int res = waAPI_queryavatar(wconn->waAPI, username, &icon, &len);
	if (res) {
		int iid = purple_imgstore_add_with_id(g_memdup(icon, len), len, NULL);
		profile_image = g_strdup_printf("<img id=\"%u\">", iid);
	}

	purple_notify_user_info_add_pair(info, "Status", status_string);
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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	GSList *buddies = purple_find_buddies(purple_connection_get_account(gc), NULL);
	GSList *l;

	for (l = buddies; l; l = l->next) {
		PurpleBuddy *b = l->data;
		const char *name = purple_buddy_get_name(b);

		waAPI_addcontact(wconn->waAPI, name);
	}

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
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	const char *groupname = g_hash_table_lookup(data, "subject");
	char *id = g_hash_table_lookup(data, "id");

	if (!id) {
		gchar *tmp = g_strdup_printf("Joining %s requires an invitation.", groupname);
		purple_notify_error(gc, "Invitation only", "Invitation only", tmp);
		g_free(tmp);
		return;
	}

	int prplid = chatid_to_convo(id);
	purple_debug_info(WHATSAPP_ID, "joining group %s\n", groupname);

	if (!purple_find_chat(gc, prplid)) {
		char *subject, *owner, *part;
		if (!waAPI_getgroupinfo(wconn->waAPI, id, &subject, &owner, &part))
			return;

		/* Notify chat add */
		PurpleConversation *conv = serv_got_joined_chat(gc, prplid, groupname);

		/* Add people in the chat */
		purple_debug_info(WHATSAPP_ID, "group info ID(%s) SUBJECT(%s) OWNER(%s)\n", id, subject, owner);
		conv_add_participants(conv, part, owner);
	}
}

static void waprpl_chat_invite(PurpleConnection * gc, int id, const char *message, const char *name)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	PurpleConversation *convo = purple_find_chat(gc, id);
	PurpleChat *ch = blist_find_chat_by_convo(gc, id);
	GHashTable *hasht = purple_chat_get_components(ch);
	char *chat_id = g_hash_table_lookup(hasht, "id");

	if (strstr(name, "@s.whatsapp.net") == 0)
		name = g_strdup_printf("%s@s.whatsapp.net", name);
	waAPI_manageparticipant(wconn->waAPI, chat_id, name, "add");

	purple_conv_chat_add_user(purple_conversation_get_chat_data(convo), name, "", PURPLE_CBFLAGS_NONE, FALSE);

	waprpl_check_output(gc);
}

static char *waprpl_get_chat_name(GHashTable * data)
{
	return g_strdup(g_hash_table_lookup(data, "subject"));
}

void waprpl_ssl_output_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	char tempbuff[16*1024];
	int ret;
	do {
		int datatosend = waAPI_sslsendcb(wconn->waAPI, tempbuff, sizeof(tempbuff));
		purple_debug_info(WHATSAPP_ID, "Output data to send %d\n", datatosend);

		if (datatosend == 0)
			break;

		ret = purple_ssl_write(wconn->gsc, tempbuff, datatosend);

		if (ret > 0) {
			waAPI_sslsenddone(wconn->waAPI, ret);
		} else if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
			/* Check later */
		} else {
			waprpl_ssl_cerr_cb(0, 0, gc);
		}
	} while (ret > 0);

	/* Check if we need to callback again or not */
	waprpl_check_ssl_output(gc);
}

/* Try to read some data and push it to the WhatsApp API */
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond)
{
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	/* End point closed the connection */
	if (!g_list_find(purple_connections_get_all(), gc)) {
		waprpl_ssl_cerr_cb(0, 0, gc);
		return;
	}

	char tempbuff[16*1024];
	int ret;
	do {
		ret = purple_ssl_read(wconn->gsc, tempbuff, sizeof(tempbuff));
		purple_debug_info(WHATSAPP_ID, "Input data read %d %d\n", ret, errno);

		if (ret > 0) {
			waAPI_sslinput(wconn->waAPI, tempbuff, ret);
		} else if (ret < 0 && errno == EAGAIN)
			break;
		else if (ret < 0) {
			waprpl_ssl_cerr_cb(0, 0, gc);
			break;
		} else {
			waprpl_ssl_cerr_cb(0, 0, gc);
		}
	} while (ret > 0);

	waprpl_check_ssl_output(gc);	/* The input data may generate responses! */
}

static void waprpl_check_complete_uploads(PurpleConnection * gc) {
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	
	GList *xfers = purple_xfers_get_all();
	while (xfers) {
		PurpleXfer *xfer = xfers->data;
		wa_file_upload *xinfo = (wa_file_upload *) xfer->data;
		if (!xinfo->done && xinfo->started && waAPI_fileuploadcomplete(wconn->waAPI, xinfo->ref_id)) {
			purple_debug_info(WHATSAPP_ID, "Upload complete\n");
			purple_xfer_set_completed(xfer, TRUE);
			xinfo->done = 1;
		}
		xfers = g_list_next(xfers);
	}
}

/* Checks if the WA protocol has data to output and schedules a write handler */
void waprpl_check_ssl_output(PurpleConnection * gc)
{
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	if (wconn->sslfd >= 0) {

		int r = waAPI_sslhasoutdata(wconn->waAPI);
		if (r > 0) {
			/* Need to watch for output data (if we are not doing it already) */
			if (wconn->sslwh == 0)
				wconn->sslwh = purple_input_add(wconn->sslfd, PURPLE_INPUT_WRITE, waprpl_ssl_output_cb, gc);
		} else if (r < 0) {
			waprpl_ssl_cerr_cb(0, 0, gc);	/* Finished the connection! */
		} else {
			if (wconn->sslwh != 0)
				purple_input_remove(wconn->sslwh);

			wconn->sslwh = 0;
		}

		purple_debug_info(WHATSAPP_ID, "Watch for output is %d %d\n", r, errno);

		/* Update transfer status */
		int rid, bytes_sent;
		if (waAPI_fileuploadprogress(wconn->waAPI, &rid, &bytes_sent)) {
			GList *xfers = purple_xfers_get_all();
			while (xfers) {
				PurpleXfer *xfer = xfers->data;
				wa_file_upload *xinfo = (wa_file_upload *) xfer->data;
				if (xinfo->ref_id == rid) {
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
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	if (!wconn) return; // The account has disconnected 

	purple_debug_info(WHATSAPP_ID, "SSL connection stablished\n");

	wconn->sslfd = gsc->fd;
	wconn->sslrh = purple_input_add(wconn->sslfd, PURPLE_INPUT_READ, waprpl_ssl_input_cb, gc);
	waprpl_check_ssl_output(gc);
}

void waprpl_ssl_cerr_cb(PurpleSslConnection * gsc, PurpleSslErrorType error, gpointer data)
{
	/* Do not use gsc, may be null */
	PurpleConnection *gc = data;
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);
	if (!wconn)
		return;

	if (wconn->sslwh != 0)
		purple_input_remove(wconn->sslwh);
	if (wconn->sslrh != 0)
		purple_input_remove(wconn->sslrh);

	waAPI_sslcloseconnection(wconn->waAPI);

	/* The connection is closed and freed automatically. */
	wconn->gsc = NULL;

	wconn->sslfd = -1;
	wconn->sslrh = 0;
	wconn->sslwh = 0;
}

void check_ssl_requests(PurpleAccount * acct)
{
	PurpleConnection *gc = purple_account_get_connection(acct);
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	char *host;
	int port;
	if (wconn->gsc == 0 && waAPI_hassslconnection(wconn->waAPI, &host, &port) > 0) {
		purple_debug_info(WHATSAPP_ID, "Establishing SSL connection to %s:%d\n", host, port);

		PurpleSslConnection *sslc = purple_ssl_connect(acct, host, port, waprpl_ssl_connected_cb, waprpl_ssl_cerr_cb, gc);
		if (sslc == 0) {
			waprpl_ssl_cerr_cb(0, 0, gc);
		} else {
			/* The Fd are not available yet, wait for connected callback */
			wconn->gsc = sslc;
		}
	}
}

void waprpl_xfer_init(PurpleXfer * xfer)
{
	purple_debug_info(WHATSAPP_ID, "File xfer init...\n");
	wa_file_upload *xinfo = (wa_file_upload *) xfer->data;
	whatsapp_connection *wconn = xinfo->wconn;

	size_t fs = purple_xfer_get_size(xfer);
	const char *fn = purple_xfer_get_filename(xfer);
	const char *fp = purple_xfer_get_local_filename(xfer);

	wa_file_upload *xfer_info = (wa_file_upload *) xfer->data;
	purple_xfer_set_size(xfer, fs);

	xfer_info->ref_id = waAPI_sendimage(wconn->waAPI, xinfo->to, 100, 100, fs, fp);
	xfer_info->started = 1;
	purple_debug_info(WHATSAPP_ID, "Transfer file %s at %s with size %zu (given ref %d)\n", fn, fp, fs, xfer_info->ref_id);

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
static PurpleXfer *waprpl_new_xfer(PurpleConnection * gc, const char *who)
{
	purple_debug_info(WHATSAPP_ID, "New file xfer\n");
	PurpleXfer *xfer = purple_xfer_new(gc->account, PURPLE_XFER_SEND, who);
	g_return_val_if_fail(xfer != NULL, NULL);
	whatsapp_connection *wconn = purple_connection_get_protocol_data(gc);

	wa_file_upload *xfer_info = g_new0(wa_file_upload, 1);
	memset(xfer_info, 0, sizeof(wa_file_upload));
	xfer_info->to = g_strdup(who);
	xfer->data = xfer_info;
	xfer_info->wconn = wconn;
	xfer_info->gc = gc;
	xfer_info->done = 0;
	xfer_info->started = 0;

	purple_xfer_set_init_fnc(xfer, waprpl_xfer_init);
	purple_xfer_set_start_fnc(xfer, waprpl_xfer_start);
	purple_xfer_set_end_fnc(xfer, waprpl_xfer_end);
	purple_xfer_set_cancel_send_fnc(xfer, waprpl_xfer_cancel_send);

	return xfer;
}

static void waprpl_send_file(PurpleConnection * gc, const char *who, const char *file)
{
	purple_debug_info(WHATSAPP_ID, "Send file called\n");
	PurpleXfer *xfer = waprpl_new_xfer(gc, who);

	if (file) {
		purple_xfer_request_accepted(xfer, file);
		purple_debug_info(WHATSAPP_ID, "Accepted transfer of file %s\n", file);
	} else
		purple_xfer_request(xfer);
}

static PurplePluginProtocolInfo prpl_info = {
	0,			/* options */
	NULL,			/* user_splits, initialized in waprpl_init() */
	NULL,			/* protocol_options, initialized in waprpl_init() */
	{			/* icon_spec, a PurpleBuddyIconSpec */
		"jpg",			/* format */
		1,			/* min_width */
		1,			/* min_height */
		640,			/* max_width */
		640,			/* max_height */
		32000,			/* max_filesize */
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
	waprpl_new_xfer,	/* new_xfer */
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

	option = purple_account_option_string_new("Server", "server", WHATSAPP_DEFAULT_SERVER);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_int_new("Port", "port", WHATSAPP_DEFAULT_PORT);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new("Nickname", "nick", "");
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	option = purple_account_option_string_new("Resource", "resource", default_resource);
	prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

	_whatsapp_protocol = plugin;

	// Some signals which can be caught by plugins
	purple_signal_register(plugin, "whatsapp-sending-message",
			purple_marshal_VOID__POINTER_POINTER_POINTER_POINTER,
			purple_value_new(PURPLE_TYPE_UNKNOWN), 4,
			purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
			purple_value_new(PURPLE_TYPE_STRING), /* id */
			purple_value_new(PURPLE_TYPE_STRING), /* who */
			purple_value_new(PURPLE_TYPE_STRING)  /* message */
	);
	purple_signal_register(plugin, "whatsapp-message-received",
			purple_marshal_VOID__POINTER_POINTER_UINT,
			purple_value_new(PURPLE_TYPE_UNKNOWN), 3,
			purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
			purple_value_new(PURPLE_TYPE_STRING),  /* id */
			purple_value_new(PURPLE_TYPE_INT)      /* reception-types */
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
	"0.1",			/* version */
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
