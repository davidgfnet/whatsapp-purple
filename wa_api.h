
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

	int waAPI_sendcb(void *waAPI, void *buffer, int maxbytes);
	void waAPI_senddone(void *waAPI, int bytessent);
	void waAPI_input(void *waAPI, const void *buffer, int bytesrecv);
	int waAPI_hasoutdata(void *waAPI);
	void waAPI_login(void *waAPI, const char *ua);
	void *waAPI_create(const char *username, const char *password, const char *nickname);
	void waAPI_delete(void *waAPI);
	void waAPI_sendim(void *waAPI, const char *who, const char *message);
	void waAPI_sendchat(void *waAPI, const char *who, const char *message);
	int waAPI_sendimage(void *waAPI, const char *who, int w, int h, unsigned int size, const char *fp);
	int waAPI_loginstatus(void *waAPI);
	void waAPI_addcontact(void *waAPI, const char *phone);
	void waAPI_delcontact(void *waAPI, const char *phone);
	int waAPI_querychat(void *waAPI, char **who, char **message, char **author, unsigned long *timestamp);
	int waAPI_querychatimage(void *waAPI, char **who, char **image, int *imglen, char **url, char **author, unsigned long *timestamp);
	int waAPI_querychatlocation(void *waAPI, char **who, char **image, int *imglen, double *lat, double *lng, char **author, unsigned long *timestamp);
	int waAPI_querychatsound(void *waAPI, char **who, char **url, char **author, unsigned long *timestamp);
	int waAPI_querystatus(void *waAPI, char **who, int *stat);
	void waAPI_sendtyping(void *waAPI, const char *who, int typing);
	void waAPI_setmypresence(void *waAPI, const char *st, const char *msg);
	int waAPI_queryicon(void *waAPI, char **who, char **icon, int *len, char **hash);
	int waAPI_queryavatar(void *waAPI, const char *who, char **icon, int *len);
	void waAPI_accountinfo(void *waAPI, unsigned long long *creation, unsigned long long *freeexpires, char **status);
	void waAPI_setavatar(void *waAPI, const void *buffer, int len);
	int waAPI_getuserstatus(void *waAPI, const char *who);
	char *waAPI_getuserstatusstring(void *waAPI, const char *who);
	unsigned long long waAPI_getlastseen(void *waAPI, const char *who);
	int waAPI_querytyping(void *waAPI, char **who, int *stat);
	char *waAPI_getgroups(void *waAPI);
	int waAPI_getgroupinfo(void *waAPI, char *id, char **subject, char **owner, char **p);
	int waAPI_getgroupbyname(void *waAPI, const char *name);
	int waAPI_getgroupsupdated(void *waAPI);
	void waAPI_creategroup(void *waAPI, const char *);
	void waAPI_deletegroup(void *waAPI, const char *);
	void waAPI_manageparticipant(void *waAPI, const char *id, const char *part, const char *command);
	int waAPI_fileuploadprogress(void *waAPI, int *rid, int *bs);

// SSL connection
	int waAPI_sslsendcb(void *waAPI, void *buffer, int maxbytes);
	void waAPI_sslsenddone(void *waAPI, int bytessent);
	void waAPI_sslinput(void *waAPI, const void *buffer, int bytesrecv);
	int waAPI_sslhasoutdata(void *waAPI);
	void waAPI_sslcloseconnection(void *waAPI);
	int waAPI_hassslconnection(void *waAPI, char **host, int *port);

// OpenSSL replacements
	unsigned char *MD5(const unsigned char *d, int n, unsigned char *md);
	int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter, int keylen, unsigned char *out);
	unsigned char *SHA1(const unsigned char *d, int n, unsigned char *md);

	const char *file_mime_type(const char *filename, const char *buf, int buflen);

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus
#include <iostream>
std::string md5hex(std::string target);
std::string md5raw(std::string target);
std::string SHA256_file_b64(const char *filename);
#endif

#endif
