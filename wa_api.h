
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

int  waAPI_sendcb(void * waAPI, void * buffer, int maxbytes);
void waAPI_senddone(void * waAPI, int bytessent);
void waAPI_input(void * waAPI, const void * buffer, int bytesrecv);
int  waAPI_hasoutdata(void * waAPI);
void waAPI_login(void * waAPI);
void * waAPI_create(const char * username, const char * password, const char * nickname);
void waAPI_delete(void * waAPI);
void waAPI_sendim(void * waAPI, const char * who, const char *message);
void waAPI_sendchat(void * waAPI, const char * who, const char *message);
int waAPI_loginstatus(void * waAPI);
void waAPI_addcontact(void * waAPI, const char * phone);
void waAPI_delcontact(void * waAPI, const char * phone);
int waAPI_querychat(void * waAPI, char ** who, char **message, char **author);
int waAPI_querychatimage(void * waAPI, char ** who, char **image, int * imglen, char ** url);
int waAPI_querychatlocation(void * waAPI, char ** who, char **image, int * imglen, double * lat, double * lng);
int waAPI_querychatsound(void * waAPI, char ** who, char ** url);
int waAPI_querystatus(void * waAPI, char ** who, int *stat);
void waAPI_sendtyping(void * waAPI,const char * who,int typing);
void waAPI_setmypresence(void * waAPI, const char * st, const char * msg);
int waAPI_queryicon(void * waAPI, char ** who, char ** icon, int * len, char ** hash);
void waAPI_accountinfo(void * waAPI, unsigned long long *creation, unsigned long long *freeexpires, char ** status);
void waAPI_setavatar(void * waAPI, const void *buffer, int len);
int waAPI_getuserstatus(void * waAPI, const char * who);
unsigned long long waAPI_getlastseen(void * waAPI, const char * who);
int waAPI_querytyping(void * waAPI, char ** who, int * stat);
char * waAPI_getgroups(void * waAPI);
int waAPI_getgroupinfo(void * waAPI, char * id, char ** subject, char ** owner, char ** p);
int waAPI_getgroupbyname(void * waAPI, const char * name);
int waAPI_getgroupsupdated(void * waAPI);
void waAPI_creategroup(void * waAPI, const char * );
void waAPI_deletegroup(void * waAPI, const char * );
void waAPI_manageparticipant(void * waAPI, const char *id, const char * part, const char * command);

// OpenSSL replacements
unsigned char *MD5(const unsigned char *d, int n, unsigned char *md);
int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter,
                           int keylen, unsigned char *out);
unsigned char *SHA1(const unsigned char *d, int n, unsigned char *md);

#ifdef __cplusplus
}
#endif


#endif


