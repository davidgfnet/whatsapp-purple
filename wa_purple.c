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

/* If you're using this as the basis of a prpl that will be distributed
 * separately from libpurple, remove the internal.h include below and replace
 * it with code to include your own config.h or similar.  If you're going to
 * provide for translation, you'll also need to setup the gettext macros. */
//#include "internal.h"

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
#else
#include <unistd.h>
#define sys_read  read
#define sys_write write
#endif

const char default_user_agent[] = "WhatsApp/2.10.750 Android/4.2.1 Device/GalaxyS3";

#define WHATSAPP_ID "prpl-whatsapp"
static PurplePlugin *_whatsapp_protocol = NULL;

#define WHATSAPP_STATUS_ONLINE   "online"
#define WHATSAPP_STATUS_AWAY     "away"
#define WHATSAPP_STATUS_OFFLINE  "offline"

#define WHATSAPP_DEFAULT_SERVER "c3.whatsapp.net"
#define WHATSAPP_DEFAULT_PORT   443

typedef struct {
  unsigned int file_size;
  char * to;
  void * wconn;
  PurpleConnection *gc;
  int ref_id;
} wa_file_upload;

typedef struct {
  PurpleAccount *account;
  int fd;         // File descriptor of the socket
  guint rh, wh;   // Read/write handlers
  int connected;  // Connection status
  void * waAPI;   // Pointer to the C++ class which actually implements the protocol
  int conv_id;    // Combo id counter
  // HTTPS interface for status query
  guint sslrh, sslwh; // Read/write handlers
  int sslfd;
  PurpleSslConnection *gsc; // SSL handler
} whatsapp_connection;

static void waprpl_check_output(PurpleConnection *gc);
static void waprpl_process_incoming_events(PurpleConnection *gc);
static void waprpl_insert_contacts(PurpleConnection *gc);
char * last_seen_text(unsigned long long t);
static void waprpl_chat_join (PurpleConnection *gc, GHashTable *data);
void check_ssl_requests(PurpleAccount *acct);
void waprpl_ssl_cerr_cb(PurpleSslConnection *gsc, PurpleSslErrorType error, gpointer data);
void waprpl_check_ssl_output(PurpleConnection *gc);
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond);

unsigned int chatid_to_convo(const char * id) {
  // Get the chat number to use as combo id
  int unused,cid;
  sscanf(id,"%d-%d",&unused,&cid);
  return cid;
}

static void waprpl_tooltip_text(PurpleBuddy *buddy, PurpleNotifyUserInfo *info, gboolean full) {
  const char * status;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
  int st = waAPI_getuserstatus(wconn->waAPI,purple_buddy_get_name(buddy));
  if (st < 0) status = "Unknown";
  else if (st == 0) status = "Unavailable";
  else status = "Available";
  unsigned long long lseen = waAPI_getlastseen(wconn->waAPI,purple_buddy_get_name(buddy));
  purple_notify_user_info_add_pair_plaintext(info, "Status", status);
  purple_notify_user_info_add_pair_plaintext(info, "Last seen on WhatsApp", purple_str_seconds_to_string(lseen));
}

static char *waprpl_status_text(PurpleBuddy *buddy) {
  char * statusmsg;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_buddy_get_account(buddy)));
  if (!wconn) return 0;

  statusmsg = waAPI_getuserstatusstring(wconn->waAPI, purple_buddy_get_name(buddy));
  if (!statusmsg || strlen(statusmsg) == 0)
    return NULL;
  return statusmsg;
}

static const char *waprpl_list_icon(PurpleAccount *acct, PurpleBuddy *buddy) {
  return "whatsapp";
}

// Show the account information received at the login
// such as expiration, creation, etc.
static void waprpl_show_accountinfo(PurplePluginAction *action) {
  PurpleConnection * gc = (PurpleConnection *) action->context;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  if (!wconn) return;
  
  unsigned long long creation, freeexpires;
  char * status;
  waAPI_accountinfo(wconn->waAPI,&creation, &freeexpires,&status);
  
  time_t creationtime = creation;
  time_t freeexpirestime = freeexpires;
  char * cr = g_strdup(asctime(localtime(&creationtime)));
  char * ex = g_strdup(asctime(localtime(&freeexpirestime)));

  char * text = g_strdup_printf("Account status: %s<br />Created on: %s<br />Free expires on: %s\n",status,cr,ex);

  purple_notify_formatted(gc, "Account information", "Account information", "", text, NULL, NULL);
}

static GList *waprpl_actions(PurplePlugin *plugin, gpointer context) {
  PurplePluginAction * act;

  act = purple_plugin_action_new("Show account information ...", waprpl_show_accountinfo);
  return g_list_append(NULL, act);
}

static int isgroup(const char *user) {
  return (strchr(user, '-') != NULL);
}

static void waprpl_blist_node_removed (PurpleBlistNode *node) {
  if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
    PurpleChat * ch = PURPLE_CHAT(node);
    char * gid = g_hash_table_lookup(purple_chat_get_components(ch), "id");
    if (gid == 0) return; // Group is not created yet...
    whatsapp_connection * wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_chat_get_account(ch)));
    waAPI_deletegroup(wconn->waAPI, gid);
    waprpl_check_output(purple_account_get_connection(purple_chat_get_account(ch)));
  }
}

static void waprpl_blist_node_added (PurpleBlistNode *node) {
  if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
    PurpleChat * ch = PURPLE_CHAT(node);
    whatsapp_connection * wconn = purple_connection_get_protocol_data(purple_account_get_connection(purple_chat_get_account(ch)));
    GHashTable * hasht = purple_chat_get_components(ch);
    const char *groupname = g_hash_table_lookup(hasht, "subject");
    const char *gid = g_hash_table_lookup(hasht, "id");
    if (gid != 0) return;  // Already created
    purple_debug_info(WHATSAPP_ID, "Creating group %s\n", groupname);
    
    waAPI_creategroup(wconn->waAPI, groupname);
    waprpl_check_output(purple_account_get_connection(purple_chat_get_account(ch)));
    
    // Remove it, it will get added at the moment the chat list gets refreshed
    purple_blist_remove_chat(ch);
  }
}

PurpleConversation * get_open_combo(const char * who, PurpleConnection *gc) {
  PurpleAccount * acc = purple_connection_get_account(gc);
  if (isgroup(who)) {
    // Search fot the combo
    PurpleBlistNode* node = purple_blist_get_root();
    GHashTable* hasht = NULL;
    while (node != 0) {
      if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
        PurpleChat * ch = PURPLE_CHAT(node);
        if (purple_chat_get_account(ch) == acc) {
          hasht = purple_chat_get_components(ch);
          if (strcmp(g_hash_table_lookup(hasht, "id"),who) == 0) {
            break;
          }
        }
      }
      node = purple_blist_node_next(node,FALSE);
    }
    int convo_id = chatid_to_convo(who);
    PurpleConversation *convo = purple_find_chat(gc, convo_id);
    
    // Create a window if it's not open yet
    if (!convo) {
      waprpl_chat_join(gc,hasht);
      convo = purple_find_chat(gc, convo_id);
    }
    return convo;
  }else{
    // Search for the combo
    PurpleConversation *convo = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, acc);
    if (!convo)
      convo = purple_conversation_new(PURPLE_CONV_TYPE_IM, acc, who);
      return convo;
  }
}

static void waprpl_process_incoming_events(PurpleConnection *gc) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  PurpleAccount * acc = purple_connection_get_account(gc);

  switch (waAPI_loginstatus(wconn->waAPI)) {
  case 0:
    purple_connection_update_progress(gc, "Connecting", 0, 4);
    break;
  case 1:
    purple_connection_update_progress(gc, "Sending auth", 1, 4);
    break;
  case 2:
    purple_connection_update_progress(gc, "Waiting response", 2, 4);
    break;
  case 3:
    purple_connection_update_progress(gc, "Connected", 3, 4);
    purple_connection_set_state(gc, PURPLE_CONNECTED);
    
    if (!wconn->connected)
      waprpl_insert_contacts(gc);
      
    wconn->connected = 1;
    break;
  default:
    break;
  };
  
  char * msg, * who, * prev, * url, *author;
  int status; int size;
  double lat,lng; unsigned long timestamp;
  // Incoming messages
  while (waAPI_querychat(wconn->waAPI, &who, &msg, &author, &timestamp)) {
    purple_debug_info(WHATSAPP_ID, "Got chat message from %s: %s\n", who,msg);
    
    PurpleConversation * convo = get_open_combo(who, gc);
    if (isgroup(who)) {
      if (convo)
        serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), author, PURPLE_MESSAGE_RECV, msg, timestamp);
    }else{
      serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), who, PURPLE_MESSAGE_RECV, msg, timestamp);
      purple_conv_im_write(PURPLE_CONV_IM(convo), who, msg, PURPLE_MESSAGE_RECV, timestamp);
    }
  }
  while (waAPI_querychatimage(wconn->waAPI, &who, &prev, &size, &url, &timestamp)) {
    purple_debug_info(WHATSAPP_ID, "Got image from %s: %s\n", who,url);
    int imgid = purple_imgstore_add_with_id(g_memdup(prev, size), size, NULL);
    
    char * msg = g_strdup_printf("<a href=\"%s\"><img id=\"%u\"></a><br/><a href=\"%s\">%s</a>",url,imgid,url,url);
    PurpleConversation * convo = get_open_combo(who, gc);
    if (isgroup(who)) {
      if (convo)
        serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), author, PURPLE_MESSAGE_RECV, msg, timestamp);
    }else{
      serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), who, PURPLE_MESSAGE_RECV, msg, timestamp);
      purple_conv_im_write(PURPLE_CONV_IM(convo), who, msg, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_IMAGES, timestamp);
    }
  }
  while (waAPI_querychatlocation(wconn->waAPI, &who, &prev, &size, &lat, &lng, &timestamp)) {
    purple_debug_info(WHATSAPP_ID, "Got geomessage from: %s Coordinates (%f %f)\n", who,(float)lat,(float)lng);
    int imgid = purple_imgstore_add_with_id(g_memdup(prev, size), size, NULL);
    char * msg =g_strdup_printf("<a href=\"http://openstreetmap.org/?lat=%f&lon=%f&zoom=16\"><img src=\"%u\"></a>",lat,lng,imgid);
    
    PurpleConversation * convo = get_open_combo(who, gc);
    if (isgroup(who)) {
      if (convo)
        serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), author, PURPLE_MESSAGE_RECV, msg, timestamp);
    }else{
      serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), who, PURPLE_MESSAGE_RECV, msg, timestamp);
      purple_conv_im_write(PURPLE_CONV_IM(convo), who, msg, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_IMAGES, timestamp);
    }
  }
  while (waAPI_querychatsound(wconn->waAPI, &who, &url, &timestamp)) {
    purple_debug_info(WHATSAPP_ID, "Got chat sound from %s: %s\n", who,url);
    char * msg = g_strdup_printf("<a href=\"%s\">%s</a>",url,url);
    
    PurpleConversation * convo = get_open_combo(who, gc);
    if (isgroup(who)) {
      if (convo)
        serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), author, PURPLE_MESSAGE_RECV, msg, timestamp);
    }else{
      serv_got_chat_in(gc, purple_conv_chat_get_id(PURPLE_CONV_CHAT(convo)), who, PURPLE_MESSAGE_RECV, msg, timestamp);
      purple_conv_im_write(PURPLE_CONV_IM(convo), who, msg, PURPLE_MESSAGE_RECV | PURPLE_MESSAGE_IMAGES, timestamp);
    }
  }
  
  // User status change
  while (waAPI_querystatus(wconn->waAPI, &who, &status)) {
    if (status == 1) {
      purple_prpl_got_user_status(acc, who, "available", "message","", NULL);
    }
    else {
      purple_prpl_got_user_status(acc, who, "unavailable", "message","", NULL);
    }
  }
  // User typing info notify
  while (waAPI_querytyping(wconn->waAPI, &who, &status)) {
    if (status == 1) {
      purple_debug_info(WHATSAPP_ID, "%s is typing\n", who);
      serv_got_typing(gc,who,0,PURPLE_TYPING);
    }
    else {
      purple_debug_info(WHATSAPP_ID, "%s is not typing\n", who);
      serv_got_typing(gc,who,0,PURPLE_NOT_TYPING);
      serv_got_typing_stopped(gc,who);
    }
  }
  
  // User profile picture
  char * icon, * hash;
  int len;
  while (waAPI_queryicon(wconn->waAPI, &who, &icon, &len, &hash)) {
    purple_buddy_icons_set_for_user(acc,who, g_memdup(icon,len),len, hash);
  }
  
  // Groups update
  if (waAPI_getgroupsupdated(wconn->waAPI)) {
  
    // Delete/update the chats that are in our list
    PurpleBlistNode* node = purple_blist_get_root();
    while (node != 0) {
      if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
        PurpleChat * ch = PURPLE_CHAT(node);
        if (purple_chat_get_account(ch) == acc) {
        
          int found = 0;
          GHashTable *hasht = purple_chat_get_components(ch);
          char * grid = g_hash_table_lookup(hasht, "id");
          char * glist = waAPI_getgroups(wconn->waAPI);
          gchar **gplist = g_strsplit(glist,",",0);
          while (*gplist) {
            if (strcmp(*gplist,grid) == 0) {
              // The group is in the system, update the fields
              char *sub,*own;
              waAPI_getgroupinfo(wconn->waAPI, *gplist, &sub, &own, 0);
              g_hash_table_insert(hasht, g_strdup("subject"), g_strdup(sub));
              g_hash_table_insert(hasht, g_strdup("owner"), g_strdup(own));
              
              found = 1;
              break;
            }
            gplist++;
          }

          // The group was deleted
          if (!found) {
              PurpleChat * del = (PurpleChat *)node;
              node = purple_blist_node_next(node,FALSE);
              purple_blist_remove_chat(del);
          }
          
        }
      }
      node = purple_blist_node_next(node,FALSE);
    }

    // Add new groups
    char * glist = waAPI_getgroups(wconn->waAPI);
    gchar **gplist = g_strsplit(glist,",",0);
    while (*gplist) {
      int found = 0;
      PurpleBlistNode* node = purple_blist_get_root();
      PurpleChat * ch;
      while (node != 0) {
        if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
          ch = PURPLE_CHAT(node);
          if (purple_chat_get_account(ch) == acc) {
            char * grid = g_hash_table_lookup(purple_chat_get_components(ch), "id");
            if (strcmp(*gplist,grid) == 0) {
              found = 1;
              break;
            }
          }
        }
        node = purple_blist_node_next(node,FALSE);
      }

      if (!found) {
        char *sub,*own;
        waAPI_getgroupinfo(wconn->waAPI, *gplist, &sub, &own, 0);
        purple_debug_info("waprpl", "New group found %s %s\n", *gplist,sub);
        
        GHashTable * htable = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
        g_hash_table_insert(htable, g_strdup("subject"), g_strdup(sub));
        g_hash_table_insert(htable, g_strdup("id"), g_strdup(*gplist));
        g_hash_table_insert(htable, g_strdup("owner"), g_strdup(own));

        ch = purple_chat_new(acc,sub,htable);
        purple_blist_add_chat(ch,NULL,NULL);
      }
      
      // Now update the open conversation that may exist
      char * id = g_hash_table_lookup(purple_chat_get_components(ch), "id");
      int prplid = chatid_to_convo(id);
      PurpleConversation * conv = purple_find_chat(gc, prplid);
      if (conv) {
        char *subject, *owner, *part;
        if (!waAPI_getgroupinfo(wconn->waAPI, id, &subject, &owner, &part)) return;
        
        purple_conv_chat_clear_users(purple_conversation_get_chat_data(conv));
        gchar **plist = g_strsplit(part,",",0);
        while (*plist) {
          purple_conv_chat_add_user (purple_conversation_get_chat_data(conv),
            *plist,"",PURPLE_CBFLAGS_NONE | (!strcmp(owner,*plist) ? PURPLE_CBFLAGS_FOUNDER : 0),FALSE);
          plist++;
        }
      }
      
      gplist++;
    }
  }
}

static void waprpl_output_cb(gpointer data, gint source, PurpleInputCondition cond) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
	
  char tempbuff[1024];
  int ret;
  do {
    int datatosend = waAPI_sendcb(wconn->waAPI,tempbuff,sizeof(tempbuff));
    if (datatosend == 0) break;
    
    ret = sys_write(wconn->fd,tempbuff,datatosend);
    
    if (ret > 0) {
      waAPI_senddone(wconn->waAPI,ret);
    }
    else if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
      // Check later
    }
    else {
      gchar *tmp = g_strdup_printf("Lost connection with server (out cb): %s",g_strerror(errno));
      purple_connection_error_reason (gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
      g_free(tmp);
      break;
    }
  } while (ret > 0);
  
  // Check if we need to callback again or not
  waprpl_check_output(gc);
}

// Try to read some data and push it to the WhatsApp API
static void waprpl_input_cb(gpointer data, gint source, PurpleInputCondition cond) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  char tempbuff[1024];
  int ret;
  do {
    ret = sys_read(wconn->fd,tempbuff,sizeof(tempbuff));
    if (ret > 0)
      waAPI_input(wconn->waAPI,tempbuff,ret);
    else if (ret < 0 && errno == EAGAIN)
      break;
    else if (ret < 0) {
      gchar *tmp = g_strdup_printf("Lost connection with server (in cb): %s",g_strerror(errno));
      purple_connection_error_reason (gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
      g_free(tmp);
      break;
    }
    else {
      purple_connection_error_reason (gc,PURPLE_CONNECTION_ERROR_NETWORK_ERROR,"Server closed the connection");
    }
  } while (ret > 0);
  
  waprpl_process_incoming_events(gc);
  waprpl_check_output(gc); // The input data may generate responses!
}

// Checks if the WA protocol has data to output and schedules a write handler
static void waprpl_check_output(PurpleConnection *gc) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  if (wconn->fd < 0) return;
    
  if (waAPI_hasoutdata(wconn->waAPI) > 0) {
    // Need to watch for output data (if we are not doing it already)
    if (wconn->wh == 0)
      wconn->wh = purple_input_add(wconn->fd, PURPLE_INPUT_WRITE, waprpl_output_cb, gc);
  }else{
    if (wconn->wh != 0)
      purple_input_remove(wconn->wh);
    
    wconn->wh = 0;
  }

  check_ssl_requests(purple_connection_get_account(gc));
}

static void waprpl_connect_cb(gpointer data, gint source, const gchar *error_message) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  PurpleAccount *acct = purple_connection_get_account(gc);
  const char *useragent = purple_account_get_string(acct, "useragent", default_user_agent);

  if (source < 0) {
    gchar *tmp = g_strdup_printf("Unable to connect: %s",error_message);
    purple_connection_error_reason (gc,PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
    g_free(tmp);
  }else{
    wconn->fd = source;
    waAPI_login(wconn->waAPI,useragent);
    wconn->rh = purple_input_add(wconn->fd, PURPLE_INPUT_READ, waprpl_input_cb, gc);
    waprpl_check_output(gc);
  }
}

static void waprpl_login(PurpleAccount *acct) {
  PurpleConnection *gc = purple_account_get_connection(acct);

  purple_debug_info("waprpl", "logging in %s\n", purple_account_get_username(acct));

  purple_connection_update_progress(gc, "Connecting", 0, 4);

  whatsapp_connection * wconn = g_new0(whatsapp_connection, 1);
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

  wconn->waAPI = waAPI_create(username,password,nickname);
  purple_connection_set_protocol_data(gc, wconn);

  const char *hostname = purple_account_get_string(acct, "server", WHATSAPP_DEFAULT_SERVER);
  int port = purple_account_get_int(acct, "port",WHATSAPP_DEFAULT_PORT);

  if (purple_proxy_connect(gc, acct, hostname, port, waprpl_connect_cb, gc) == NULL) {
    purple_connection_error_reason (gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR, "Unable to connect");
  }

  static int sig_con = 0;
  if (!sig_con) {
    sig_con = 1;
  	purple_signal_connect(purple_blist_get_handle(), "blist-node-removed", _whatsapp_protocol,
	    PURPLE_CALLBACK(waprpl_blist_node_removed),NULL);
  	purple_signal_connect(purple_blist_get_handle(), "blist-node-added", _whatsapp_protocol,
	    PURPLE_CALLBACK(waprpl_blist_node_added),NULL);
	}
}

static void waprpl_close(PurpleConnection *gc) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  
  if (wconn->rh)
    purple_input_remove(wconn->rh);
  if (wconn->wh)
    purple_input_remove(wconn->wh);
  
  if (wconn->fd >= 0)
    close(wconn->fd);
  
  if (wconn->waAPI)
    waAPI_delete(wconn->waAPI);
  wconn->waAPI = NULL;

  g_free(wconn);
  purple_connection_set_protocol_data(gc, 0);
}

static int waprpl_send_im(PurpleConnection *gc, const char *who, const char *message, PurpleMessageFlags flags) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  waAPI_sendim(wconn->waAPI,who,message);
  waprpl_check_output(gc);
  
  return 1;
}
static int waprpl_send_chat(PurpleConnection *gc, int id, const char *message, PurpleMessageFlags flags) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  PurpleAccount *account = purple_connection_get_account(gc);
  PurpleConversation *convo = purple_find_chat(gc, id);
  
  PurpleBlistNode* node = purple_blist_get_root();
  GHashTable* hasht = NULL;
  while (node != 0) {
    if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
      PurpleChat * ch = PURPLE_CHAT(node);
      if (purple_chat_get_account(ch) == account) {
        hasht = purple_chat_get_components(ch);
        if (chatid_to_convo(g_hash_table_lookup(hasht, "id")) == (unsigned)id) {
          break;
        }
      }
    }
    node = purple_blist_node_next(node,FALSE);
  }

  char * chat_id = g_hash_table_lookup(hasht, "id");
  waAPI_sendchat(wconn->waAPI,chat_id,message);
  waprpl_check_output(gc);

  const char *me = purple_account_get_string(account, "nick", "");
  purple_conv_chat_write(PURPLE_CONV_CHAT(convo), me, message, PURPLE_MESSAGE_SEND, time(NULL));

  return 1;
}


static void waprpl_add_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  const char * name = purple_buddy_get_name(buddy);
  
  waAPI_addcontact(wconn->waAPI,name);
  
  waprpl_check_output(gc);
}

static void waprpl_add_buddies(PurpleConnection *gc, GList *buddies, GList *groups) {
  GList *buddy = buddies;
  GList *group = groups;

  while (buddy && group) {
    waprpl_add_buddy(gc, (PurpleBuddy *)buddy->data, (PurpleGroup *)group->data);
    buddy = g_list_next(buddy);
    group = g_list_next(group);
  }
}

static void waprpl_remove_buddy(PurpleConnection *gc, PurpleBuddy *buddy, PurpleGroup *group) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  const char * name = purple_buddy_get_name(buddy);
  
  waAPI_delcontact(wconn->waAPI,name);
  
  waprpl_check_output(gc);
}

static void waprpl_remove_buddies(PurpleConnection *gc, GList *buddies, GList *groups) {
  GList *buddy = buddies;
  GList *group = groups;

  while (buddy && group) {
    waprpl_remove_buddy(gc, (PurpleBuddy *)buddy->data, (PurpleGroup *)group->data);
    buddy = g_list_next(buddy);
    group = g_list_next(group);
  }
}

static void waprpl_convo_closed(PurpleConnection *gc, const char *who) {
  // TODO
}

static void waprpl_add_deny(PurpleConnection *gc, const char *name) {
  // TODO Do we need to implement deny? Or purple provides it?
}

static void waprpl_rem_deny(PurpleConnection *gc, const char *name) {
  // TODO Do we need to implement deny? Or purple provides it?
}

static unsigned int waprpl_send_typing(PurpleConnection *gc, const char *who, PurpleTypingState typing) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  
  int status = 0;
  if (typing == PURPLE_TYPING) status = 1;

  purple_debug_info(WHATSAPP_ID, "purple: %s typing status: %d\n", who, typing);
  
  waAPI_sendtyping(wconn->waAPI,who,status);
  waprpl_check_output(gc);
  
  return 1;
}

static void waprpl_set_buddy_icon(PurpleConnection *gc, PurpleStoredImage *img) {
  // Send the picture the user has selected!
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  size_t size = purple_imgstore_get_size(img);
  const void * data = purple_imgstore_get_data(img);
  waAPI_setavatar(wconn->waAPI, data, size);
  
  waprpl_check_output(gc);
}

static gboolean waprpl_can_receive_file(PurpleConnection *gc, const char *who) {
  return TRUE;
}

static gboolean waprpl_offline_message(const PurpleBuddy *buddy) {
  return FALSE;
}

static GList *waprpl_status_types(PurpleAccount *acct) {
  GList *types = NULL;
  PurpleStatusType *type;

  type = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE,
      "available", NULL, TRUE, TRUE, FALSE,
      "message", "Message", purple_value_new(PURPLE_TYPE_STRING), NULL);
  types = g_list_prepend(types, type);

  type = purple_status_type_new_with_attrs(PURPLE_STATUS_AWAY,
      "unavailable", NULL, TRUE, TRUE, FALSE,
      "message", "Message", purple_value_new(PURPLE_TYPE_STRING), NULL);
  types = g_list_prepend(types, type);

  return g_list_reverse(types);
}

static GList *waprpl_blist_node_menu(PurpleBlistNode *node) {
  if (PURPLE_BLIST_NODE_IS_BUDDY(node)) {
    return NULL;
  } else {
    return NULL;
  }
}

static void waprpl_set_status(PurpleAccount *acct, PurpleStatus *status) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(purple_account_get_connection(acct));
  const char * sid = purple_status_get_id(status);
  const char * mid = purple_status_get_attr_string(status, "message");
  if (mid == 0) mid = "";
  
  waAPI_setmypresence(wconn->waAPI,sid,mid);
  waprpl_check_output(purple_account_get_connection(acct));
}

static void waprpl_get_info(PurpleConnection *gc, const char *username) {
  PurpleNotifyUserInfo *info = purple_notify_user_info_new();
  purple_debug_info(WHATSAPP_ID, "Fetching %s's user info for %s\n", username, gc->account->username);
  
  // Get user status
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  const char * status_string = waAPI_getuserstatusstring(wconn->waAPI,username);
  // Get user picture (big one)
  char * profile_image = "";
  char *icon; int len;
  int res = waAPI_queryavatar(wconn->waAPI, username, &icon, &len);
  if (res) {
    int iid = purple_imgstore_add_with_id(g_memdup(icon,len),len,NULL);
    profile_image = g_strdup_printf("<img id=\"%u\">",iid);
  }

  purple_notify_user_info_add_pair(info, "Status", status_string);
  purple_notify_user_info_add_pair(info, "Profile image", profile_image);

  purple_notify_userinfo(gc, username, info, NULL, NULL);
}

static void waprpl_group_buddy(PurpleConnection *gc, const char *who, const char *old_group, const char *new_group) {
  // TODO implement local groups
}

static void waprpl_rename_group(PurpleConnection *gc, const char *old_name, PurpleGroup *group, GList *moved_buddies) {
  // TODO implement local groups
}

static void waprpl_insert_contacts(PurpleConnection *gc) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  GSList *buddies = purple_find_buddies(purple_connection_get_account(gc), NULL);
  GSList * l;

  for (l = buddies; l; l = l->next) {
    PurpleBuddy * b = l->data;
    const char * name = purple_buddy_get_name(b);
  
    waAPI_addcontact(wconn->waAPI,name);
  }
  
  waprpl_check_output(gc);
  
  g_slist_free(buddies);
}

// WA group support as chats
static GList *waprpl_chat_join_info(PurpleConnection *gc) {
  struct proto_chat_entry *pce;

  pce = g_new0(struct proto_chat_entry, 1);
  pce->label = "_Subject:";
  pce->identifier = "subject";
  pce->required = TRUE;
  return g_list_append(NULL, pce);
}

static GHashTable *waprpl_chat_info_defaults(PurpleConnection *gc, const char *chat_name) {
  GHashTable *defaults = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  if (chat_name != NULL)
    g_hash_table_insert(defaults, g_strdup("subject"), g_strdup(chat_name));

  return defaults;
}

static void waprpl_chat_join (PurpleConnection *gc, GHashTable *data) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  const char *groupname = g_hash_table_lookup(data, "subject");
  char * id = g_hash_table_lookup(data, "id");
  int prplid = chatid_to_convo(id);
  purple_debug_info(WHATSAPP_ID, "joining group %s\n", groupname);
  
  if (!purple_find_chat(gc, prplid)) {
    // Notify chat add
    PurpleConversation * conv = serv_got_joined_chat(gc, prplid, groupname);
    
    // Add people in the chat

    char *subject, *owner, *part;
    if (!waAPI_getgroupinfo(wconn->waAPI, id, &subject, &owner, &part)) return;
    purple_debug_info(WHATSAPP_ID, "group info ID(%s) SUBJECT(%s) OWNER(%s)\n", id, subject, owner);
    
    gchar **plist = g_strsplit(part,",",0);
    while (*plist) {
      purple_conv_chat_add_user (purple_conversation_get_chat_data(conv),
        *plist,"",PURPLE_CBFLAGS_NONE | (!strcmp(owner,*plist) ? PURPLE_CBFLAGS_FOUNDER : 0),FALSE);
      plist++;
    }
  }
}

static void waprpl_chat_invite(PurpleConnection *gc, int id, const char *message, const char *name) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  PurpleAccount *account = purple_connection_get_account(gc);
  PurpleConversation *convo = purple_find_chat(gc, id);
  
  PurpleBlistNode* node = purple_blist_get_root();
  GHashTable* hasht = NULL;
  while (node != 0) {
    if (PURPLE_BLIST_NODE_IS_CHAT(node)) {
      PurpleChat * ch = PURPLE_CHAT(node);
      if (purple_chat_get_account(ch) == account) {
        hasht = purple_chat_get_components(ch);
        if (chatid_to_convo(g_hash_table_lookup(hasht, "id")) == (unsigned)id) {
          break;
        }
      }
    }
    node = purple_blist_node_next(node,FALSE);
  }

  char * chat_id = g_hash_table_lookup(hasht, "id");
  if (strstr(name,"@s.whatsapp.net") == 0) name = g_strdup_printf("%s@s.whatsapp.net",name);
  waAPI_manageparticipant(wconn->waAPI, chat_id, name, "add");
  
  purple_conv_chat_add_user (purple_conversation_get_chat_data(convo), name, "", PURPLE_CBFLAGS_NONE, FALSE);

  waprpl_check_output(gc);
}

static char *waprpl_get_chat_name(GHashTable *data) {
  return g_strdup(g_hash_table_lookup(data, "subject"));
}

void waprpl_ssl_output_cb(gpointer data, gint source, PurpleInputCondition cond) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
	
  char tempbuff[1024];
  int ret;
  do {
    int datatosend = waAPI_sslsendcb(wconn->waAPI,tempbuff,sizeof(tempbuff));
    if (datatosend == 0) break;
    
    ret = purple_ssl_write(wconn->gsc, tempbuff, datatosend);
    
    if (ret > 0) {
      waAPI_sslsenddone(wconn->waAPI,ret);
    }
    else if (ret == 0 || (ret < 0 && errno == EAGAIN)) {
      // Check later
    }
    else {
      waprpl_ssl_cerr_cb(0,0,gc);
    }
  } while (ret > 0);
  
  // Check if we need to callback again or not
  waprpl_check_ssl_output(gc);
}

// Try to read some data and push it to the WhatsApp API
void waprpl_ssl_input_cb(gpointer data, gint source, PurpleInputCondition cond) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  // End point closed the connection
  if(!g_list_find(purple_connections_get_all(), gc)) {
   waprpl_ssl_cerr_cb(0,0,gc);
   return;
  }

  char tempbuff[1024];
  int ret;
  do {
    ret = purple_ssl_read(wconn->gsc, tempbuff, sizeof(tempbuff));
    if (ret > 0) {
      waAPI_sslinput(wconn->waAPI,tempbuff,ret);
    }else if (ret < 0 && errno == EAGAIN)
      break;
    else if (ret < 0) {
      waprpl_ssl_cerr_cb(0,0,gc);
      break;
    }
    else {
      waprpl_ssl_cerr_cb(0,0,gc);
    }
  } while (ret > 0);
  
  //waprpl_process_ssl_events(gc);
  waprpl_check_ssl_output(gc); // The input data may generate responses!
}

// Checks if the WA protocol has data to output and schedules a write handler
void waprpl_check_ssl_output(PurpleConnection *gc) {
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  if (wconn->sslfd < 0) return;
    
  int r = waAPI_sslhasoutdata(wconn->waAPI);
  if (r > 0) {
    // Need to watch for output data (if we are not doing it already)
    if (wconn->sslwh == 0)
      wconn->sslwh = purple_input_add(wconn->sslfd, PURPLE_INPUT_WRITE, waprpl_ssl_output_cb, gc);
  }
  else if (r < 0) {
    waprpl_ssl_cerr_cb(0,0,gc);  // Finished the connection!
  }
  else{
    if (wconn->sslwh != 0)
      purple_input_remove(wconn->sslwh);
    
    wconn->sslwh = 0;
  }

  // Update transfer status
  int rid, bytes_sent;
  if (waAPI_fileuploadprogress(wconn->waAPI,&rid,&bytes_sent)) {
    GList * xfers = purple_xfers_get_all();
    while (xfers) {
      PurpleXfer* xfer = xfers->data;
      wa_file_upload * xinfo = (wa_file_upload*)xfer->data;
      if (xinfo->ref_id == rid) {
        purple_debug_info("waprpl", "Upload progress %d bytes done\n",bytes_sent);
        purple_xfer_set_bytes_sent (xfer, bytes_sent);
        purple_xfer_update_progress(xfer);
        if (bytes_sent >= (signed)purple_xfer_get_size(xfer))
          purple_xfer_set_completed(xfer,TRUE);
        break;
      }
      xfers = g_list_next(xfers);
    }
  }
}

static void waprpl_ssl_connected_cb(gpointer data, PurpleSslConnection *gsc, PurpleInputCondition cond) {
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  purple_debug_info("waprpl", "SSL connection stablished\n");

  wconn->sslfd = gsc->fd;
  wconn->sslrh = purple_input_add(wconn->sslfd, PURPLE_INPUT_READ, waprpl_ssl_input_cb, gc);
  waprpl_check_ssl_output(gc);
}

void waprpl_ssl_cerr_cb(PurpleSslConnection *gsc, PurpleSslErrorType error, gpointer data) {
  // Do not use gsc, may be null
  PurpleConnection *gc = data;
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);
  if (!wconn) return;

  if (wconn->sslwh != 0) purple_input_remove(wconn->sslwh);
  if (wconn->sslrh != 0) purple_input_remove(wconn->sslrh);

  waAPI_sslcloseconnection(wconn->waAPI);
  if (wconn->gsc != 0)
    purple_ssl_close(gsc);

  wconn->gsc = 0;
  wconn->sslfd = -1;
  wconn->sslrh = 0;
  wconn->sslwh = 0;
}

void check_ssl_requests(PurpleAccount *acct) {
  PurpleConnection *gc = purple_account_get_connection(acct);
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  char * host; int port;
  if (wconn->gsc == 0 && waAPI_hassslconnection(wconn->waAPI,&host,&port) > 0) {
    purple_debug_info("waprpl", "Establishing SSL connection to %s:%d\n",host,port);

    PurpleSslConnection * sslc=purple_ssl_connect (acct, host, port, waprpl_ssl_connected_cb, waprpl_ssl_cerr_cb, gc);
    if (sslc == 0) {
      waprpl_ssl_cerr_cb(0,0,gc);
    }else{
      // The Fd are not available yet, wait for connected callback
      wconn->gsc = sslc;
    }
  }
}

void waprpl_xfer_init(PurpleXfer *xfer) {
  purple_debug_info(WHATSAPP_ID, "File xfer init...\n");
  wa_file_upload * xinfo = (wa_file_upload*)xfer->data;
  whatsapp_connection * wconn = xinfo->wconn;

  size_t fs = purple_xfer_get_size (xfer);
  const char * fn = purple_xfer_get_filename(xfer);
  const char * fp = purple_xfer_get_local_filename(xfer);

  wa_file_upload * xfer_info = (wa_file_upload *)xfer->data;
  purple_xfer_set_size (xfer, fs);

  xfer_info->ref_id = waAPI_sendimage(wconn->waAPI, xinfo->to, 100,100,fs,fp);
  purple_debug_info(WHATSAPP_ID, "Transfer file %s at %s with size %zu (given ref %d)\n", fn, fp, fs, xfer_info->ref_id);

  waprpl_check_output(xinfo->gc);
}

void waprpl_xfer_start (PurpleXfer * xfer) {
  purple_debug_info(WHATSAPP_ID, "Starting file tranfer...\n");
}

void waprpl_xfer_end (PurpleXfer * xfer) {
  purple_debug_info(WHATSAPP_ID, "Ended file tranfer!\n");
}

void waprpl_xfer_cancel_send (PurpleXfer * xfer) {
  purple_debug_info(WHATSAPP_ID, "File tranfer cancel send!!!\n");
  // TODO: Add cancel call, should be pretty easy
}


// Send file (used for sending images?)
static PurpleXfer* waprpl_new_xfer(PurpleConnection *gc, const char *who) {
  purple_debug_info(WHATSAPP_ID, "New file xfer\n");
  PurpleXfer * xfer = purple_xfer_new(gc->account, PURPLE_XFER_SEND, who);
  g_return_val_if_fail(xfer != NULL, NULL);
  whatsapp_connection * wconn = purple_connection_get_protocol_data(gc);

  wa_file_upload * xfer_info = g_new0(wa_file_upload, 1);
  memset(xfer_info,0,sizeof(wa_file_upload));
  xfer_info->to = g_strdup(who);
  xfer->data = xfer_info;
  xfer_info->wconn = wconn;
  xfer_info->gc = gc;

  purple_xfer_set_init_fnc(xfer, waprpl_xfer_init);
  purple_xfer_set_start_fnc(xfer, waprpl_xfer_start);
  purple_xfer_set_end_fnc(xfer, waprpl_xfer_end);
  purple_xfer_set_cancel_send_fnc(xfer, waprpl_xfer_cancel_send);

  return xfer;
}

static void waprpl_send_file(PurpleConnection *gc, const char *who, const char *file) {
  purple_debug_info(WHATSAPP_ID, "Send file called\n");
  PurpleXfer *xfer = waprpl_new_xfer(gc, who);

  if (file) {
    purple_xfer_request_accepted(xfer, file);
    purple_debug_info(WHATSAPP_ID, "Accepted transfer of file %s\n",file);
  }
  else
    purple_xfer_request(xfer);
}

static PurplePluginProtocolInfo prpl_info =
{
  0,                                   /* options */
  NULL,                                /* user_splits, initialized in waprpl_init() */
  NULL,                                /* protocol_options, initialized in waprpl_init() */
  {                                    /* icon_spec, a PurpleBuddyIconSpec */
      "png",                           /* format */
      1,                               /* min_width */
      1,                               /* min_height */
      512,                             /* max_width */
      512,                             /* max_height */
      64000,                           /* max_filesize */
      PURPLE_ICON_SCALE_SEND,          /* scale_rules */
  },
  waprpl_list_icon,                    /* list_icon */
  NULL,                                /* list_emblem */
  waprpl_status_text,                  /* status_text */
  waprpl_tooltip_text,                 /* tooltip_text */
  waprpl_status_types,                 /* status_types */
  waprpl_blist_node_menu,              /* blist_node_menu */
  waprpl_chat_join_info,               /* chat_info */
  waprpl_chat_info_defaults,           /* chat_info_defaults */
  waprpl_login,                        /* login */
  waprpl_close,                        /* close */
  waprpl_send_im,                      /* send_im */
  NULL,                                /* set_info */
  waprpl_send_typing,                  /* send_typing */
  waprpl_get_info,                     /* get_info */
  waprpl_set_status,                   /* set_status */
  NULL,                                /* set_idle */
  NULL,                                /* change_passwd */
  waprpl_add_buddy,                    /* add_buddy */
  waprpl_add_buddies,                  /* add_buddies */
  waprpl_remove_buddy,                 /* remove_buddy */
  waprpl_remove_buddies,               /* remove_buddies */
  NULL,                                /* add_permit */
  waprpl_add_deny,                     /* add_deny */
  NULL,                                /* rem_permit */
  waprpl_rem_deny,                     /* rem_deny */
  NULL,                                /* set_permit_deny */
  waprpl_chat_join,                    /* join_chat */
  NULL,                                /* reject_chat */
  waprpl_get_chat_name,                /* get_chat_name */
  waprpl_chat_invite,                  /* chat_invite */
  NULL,                                /* chat_leave */
  NULL,                                /* chat_whisper */
  waprpl_send_chat,                    /* chat_send */
  NULL,                                /* keepalive */
  NULL,                                /* register_user */
  NULL,                                /* get_cb_info */
  NULL,                                /* get_cb_away */
  NULL,                                /* alias_buddy */
  waprpl_group_buddy,                  /* group_buddy */
  waprpl_rename_group,                 /* rename_group */
  NULL,                                /* buddy_free */
  waprpl_convo_closed,                 /* convo_closed */
  purple_normalize_nocase,             /* normalize */
  waprpl_set_buddy_icon,               /* set_buddy_icon */
  NULL,                                /* remove_group */
  NULL,                                /* get_cb_real_name */
  NULL,                                /* set_chat_topic */
  NULL,                                /* find_blist_chat */
  NULL,                                /* roomlist_get_list */
  NULL,                                /* roomlist_cancel */
  NULL,                                /* roomlist_expand_category */
  waprpl_can_receive_file,             /* can_receive_file */
  waprpl_send_file,                    /* send_file */
  waprpl_new_xfer,                     /* new_xfer */
  waprpl_offline_message,              /* offline_message */
  NULL,                                /* whiteboard_prpl_ops */
  NULL,                                /* send_raw */
  NULL,                                /* roomlist_room_serialize */
  NULL,                                /* unregister_user */
  NULL,                                /* send_attention */
  NULL,                                /* get_attention_types */
  sizeof(PurplePluginProtocolInfo),    /* struct_size */
  NULL,                                /* get_account_text_table */
  NULL,                                /* initiate_media */
  NULL,                                /* get_media_caps */
  NULL,                                /* get_moods */
  NULL,                                /* set_public_alias */
  NULL,                                /* get_public_alias */
  NULL,                                /* add_buddy_with_invite */
  NULL                                 /* add_buddies_with_invite */
};

static void waprpl_init(PurplePlugin *plugin)
{
  PurpleAccountOption *option;

  prpl_info.user_splits = NULL;

  option = purple_account_option_string_new("Server", "server", WHATSAPP_DEFAULT_SERVER);
  prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

  option = purple_account_option_int_new("Port", "port", WHATSAPP_DEFAULT_PORT);
  prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

  option = purple_account_option_string_new("Nickname", "nick", "");
  prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);

  option = purple_account_option_string_new("User Agent", "useragent", default_user_agent);
  prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
  
  _whatsapp_protocol = plugin;
}

static PurplePluginInfo info =
{
  PURPLE_PLUGIN_MAGIC,                                     /* magic */
  PURPLE_MAJOR_VERSION,                                    /* major_version */
  PURPLE_MINOR_VERSION,                                    /* minor_version */
  PURPLE_PLUGIN_PROTOCOL,                                  /* type */
  NULL,                                                    /* ui_requirement */
  0,                                                       /* flags */
  NULL,                                                    /* dependencies */
  PURPLE_PRIORITY_DEFAULT,                                 /* priority */
  WHATSAPP_ID,                                             /* id */
  "WhatsApp",                                              /* name */
  "0.1",                                                   /* version */
  "WhatsApp protocol for libpurple",                       /* summary */
  "WhatsApp protocol for libpurple",                       /* description */
  "David Guillen Fandos (david@davidgf.net)",              /* author */
  "http://davidgf.net",                                    /* homepage */
  NULL,                                                    /* load */
  NULL,                                                    /* unload */
  NULL,                                                    /* destroy */
  NULL,                                                    /* ui_info */
  &prpl_info,                                              /* extra_info */
  NULL,                                                    /* prefs_info */
  waprpl_actions,                                          /* actions */
  NULL,                                                    /* padding... */
  NULL,
  NULL,
  NULL,
};

PURPLE_INIT_PLUGIN(whatsapp, waprpl_init, info);
