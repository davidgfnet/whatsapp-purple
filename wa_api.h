
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#ifndef WA_API_H__
#define WA_API_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int type;

	char * who;
	char * message;
	char * author;
	unsigned long t;

	char * image;
	int imagelen;
	char * url;

	double lat, lng;
} t_message;

	int waAPI_queryreceivedmsg(void *waAPI, char * id, int * type, char * from);
	int waAPI_querymsg(void *waAPI, t_message * msg);

	int waAPI_sendcb(void *waAPI, void *buffer, int maxbytes);
	void waAPI_senddone(void *waAPI, int bytessent);
	void waAPI_input(void *waAPI, const void *buffer, int bytesrecv);
	int waAPI_hasoutdata(void *waAPI);
	void waAPI_login(void *waAPI, const char *ua);
	void *waAPI_create(const char *username, const char *password, const char *nickname);
	void waAPI_delete(void *waAPI);
	void waAPI_getmsgid(void *waAPI, char * msgid);
	void waAPI_sendim(void *waAPI, const char *id, const char *who, const char *message);
	void waAPI_sendchat(void *waAPI, const char *id, const char *who, const char *message);
	int waAPI_sendimage(void *waAPI, const char *id, const char *who, int w, int h, unsigned int size, const char *fp);
	int waAPI_loginstatus(void *waAPI);
	void waAPI_addcontact(void *waAPI, const char *phone);
	void waAPI_contactsupdate(void *waAPI);
	void waAPI_delcontact(void *waAPI, const char *phone);
	int waAPI_querystatus(void *waAPI, char **who, int *stat);
	void waAPI_sendtyping(void *waAPI, const char *who, int typing);
	void waAPI_setmypresence(void *waAPI, const char *st, const char *msg);
	int waAPI_queryicon(void *waAPI, char **who, char **icon, int *len, char **hash);
	int waAPI_queryavatar(void *waAPI, const char *who, char **icon, int *len);
	void waAPI_accountinfo(void *waAPI, unsigned long long *creation, unsigned long long *freeexpires, char **status);
	void waAPI_setavatar(void *waAPI, const void *buffer, int len, const void *buffers, int lens);
	int waAPI_getuserstatus(void *waAPI, const char *who);
	char *waAPI_getuserstatusstring(void *waAPI, const char *who);
	unsigned long long waAPI_getlastseen(void *waAPI, const char *who);
	int waAPI_querytyping(void *waAPI, char **who, int *stat);
	char *waAPI_getgroups(void *waAPI);
	int waAPI_getgroupinfo(void *waAPI, const char *id, char **subject, char **owner, char **p);
	int waAPI_getgroupbyname(void *waAPI, const char *name);
	int waAPI_getgroupsupdated(void *waAPI);
	void waAPI_creategroup(void *waAPI, const char *);
	void waAPI_deletegroup(void *waAPI, const char *);
	void waAPI_manageparticipant(void *waAPI, const char *id, const char *part, const char *command);
	int waAPI_fileuploadprogress(void *waAPI, int *rid, int *bs);
	int waAPI_fileuploadcomplete(void *waAPI, int rid);
	void waAPI_queryprivacy(void *waAPI, char *, char *, char*);
	void waAPI_setprivacy(void *waAPI, const char *, const char *, const char*);
	int waAPI_geterror(void *waAPI, char **);

// SSL connection
	int waAPI_sslsendcb(void *waAPI, void *buffer, int maxbytes);
	void waAPI_sslsenddone(void *waAPI, int bytessent);
	void waAPI_sslinput(void *waAPI, const void *buffer, int bytesrecv);
	int waAPI_sslhasoutdata(void *waAPI);
	void waAPI_sslcloseconnection(void *waAPI);
	int waAPI_hassslconnection(void *waAPI, char **host, int *port);

#ifdef __cplusplus
}
#endif



#endif
