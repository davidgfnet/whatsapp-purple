
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#include "wa_api.h"
#include <cipher.h>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <stdio.h>

class WhatsappConnection;

class Group {
public:
	Group(std::string id, std::string subject, std::string owner) {
		this->id = id;
		this->subject = subject;
		this->owner = owner;
	}
	~Group() {}
	std::string id,subject,owner;
	std::vector <std::string> participants;
};

class WhatsappConnectionAPI {
private:
	WhatsappConnection * connection;

public:
	WhatsappConnectionAPI(std::string phone, std::string password, std::string nick);
	~WhatsappConnectionAPI();
	
	void doLogin(std::string);
	void receiveCallback(const char * data, int len);
	int  sendCallback(char * data, int len);
	void sentCallback(int len);
	bool hasDataToSend();
	
	void addContacts(std::vector <std::string> clist);
	void sendChat(std::string to, std::string message);
	void sendGroupChat(std::string to, std::string message);
	bool query_chat(std::string & from, std::string & message,std::string & author, unsigned long & t);
	bool query_chatimages(std::string & from, std::string & preview, std::string & url, unsigned long & t);
	bool query_chatlocations(std::string & from, double & lat, double & lng, std::string & preview, unsigned long & t);
	bool query_chatsounds(std::string & from, std::string & url, unsigned long & t);
	bool query_status(std::string & from, int & stat);
	bool query_typing(std::string & from, int & status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	void account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status);
	void send_avatar(const std::string & avatar);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);
	std::map <std::string,Group> getGroups();
	bool groupsUpdated();
	void leaveGroup(std::string group);
	void addGroup(std::string subject);
	void manageParticipant(std::string,std::string,std::string);
	
	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);
	
	int loginStatus() const;
	int sendImage(std::string to, int w, int h, unsigned int size, const char * fp);

	int sendSSLCallback(char* buffer, int maxbytes);
	int sentSSLCallback(int bytessent);
	void receiveSSLCallback(char* buffer, int bytesrecv);
	bool hasSSLDataToSend();
	bool closeSSLConnection();
	void SSLCloseCallback();
	bool hasSSLConnection(std::string & host, int * port);
};

char * waAPI_getgroups(void * waAPI) {
	std::map <std::string,Group> g = ((WhatsappConnectionAPI*)waAPI)->getGroups();
	std::string ids;
	for (std::map<std::string,Group>::iterator it = g.begin(); it != g.end(); it++) {
		if (it != g.begin()) ids += ",";
		ids += it->first;
	}
	return g_strdup(ids.c_str());
}

int waAPI_getgroupsupdated(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->groupsUpdated()) return 1;
	return 0;
}

int waAPI_getgroupinfo(void * waAPI, char * id, char ** subject, char ** owner, char ** p) {
	std::map <std::string,Group> ret = ((WhatsappConnectionAPI*)waAPI)->getGroups();
	
	std::string sid = std::string(id);
	if (ret.find(sid) == ret.end()) return 0;
	
	std::string part;
	for (unsigned int i = 0; i < ret.at(sid).participants.size(); i++) {
		if (i != 0) part += ",";
		part += ret.at(sid).participants[i];
	}
	
	if (subject) *subject = g_strdup(ret.at(sid).subject.c_str());
	if (owner)   *owner = g_strdup(ret.at(sid).owner.c_str());
	if (p)       *p = g_strdup(part.c_str());
	
	return 1;
}
void waAPI_creategroup(void * waAPI, const char *subject) {
	((WhatsappConnectionAPI*)waAPI)->addGroup(std::string(subject));
}
void waAPI_deletegroup(void * waAPI, const char * subject) {
	((WhatsappConnectionAPI*)waAPI)->leaveGroup(std::string(subject));
}
void waAPI_manageparticipant(void * waAPI, const char *id, const char * part, const char * command) {
	((WhatsappConnectionAPI*)waAPI)->manageParticipant(std::string(id),std::string(part),std::string(command));
}

void waAPI_setavatar(void * waAPI, const void *buffer,int len) {
	std::string im((const char*)buffer,(size_t)len);
	((WhatsappConnectionAPI*)waAPI)->send_avatar(im);
}

int  waAPI_sendcb(void * waAPI, void * buffer, int maxbytes) {
	return ((WhatsappConnectionAPI*)waAPI)->sendCallback((char*)buffer,maxbytes);
}

void waAPI_senddone(void * waAPI, int bytessent) {
	return ((WhatsappConnectionAPI*)waAPI)->sentCallback(bytessent);
}

void waAPI_input(void * waAPI, const void * buffer, int bytesrecv) {
	((WhatsappConnectionAPI*)waAPI)->receiveCallback((char*)buffer,bytesrecv);
}

int  waAPI_hasoutdata(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->hasDataToSend()) return 1;
	return 0;
}


int  waAPI_sslsendcb(void * waAPI, void * buffer, int maxbytes) {
	return ((WhatsappConnectionAPI*)waAPI)->sendSSLCallback((char*)buffer,maxbytes);
}

void waAPI_sslsenddone(void * waAPI, int bytessent) {
	((WhatsappConnectionAPI*)waAPI)->sentSSLCallback(bytessent);
}

void waAPI_sslinput(void * waAPI, const void * buffer, int bytesrecv) {
	((WhatsappConnectionAPI*)waAPI)->receiveSSLCallback((char*)buffer,bytesrecv);
}

int  waAPI_sslhasoutdata(void * waAPI) {
	if (((WhatsappConnectionAPI*)waAPI)->hasSSLDataToSend()) return 1;
	if (((WhatsappConnectionAPI*)waAPI)->closeSSLConnection()) return -1;
	return 0;
}

int waAPI_hassslconnection(void * waAPI, char ** host, int * port) {
	std::string shost;
	bool r = ((WhatsappConnectionAPI*)waAPI)->hasSSLConnection(shost,port);
	if (r)
		*host = (char*)g_strdup(shost.c_str());
	return r;
}

void waAPI_sslcloseconnection(void * waAPI) {
	((WhatsappConnectionAPI*)waAPI)->SSLCloseCallback();
}



void waAPI_login(void * waAPI, const char * ua) {
	((WhatsappConnectionAPI*)waAPI)->doLogin(ua);
}

void * waAPI_create(const char * username, const char * password, const char * nickname) {
	WhatsappConnectionAPI * api = new WhatsappConnectionAPI (username,password,nickname);
	return api;
}

void waAPI_delete(void * waAPI) {
	delete ((WhatsappConnectionAPI*)waAPI);
}

void waAPI_sendim(void * waAPI, const char * who, const char *message) {
	((WhatsappConnectionAPI*)waAPI)->sendChat(std::string(who),std::string(message));
}
void waAPI_sendchat(void * waAPI, const char * who, const char *message) {
	((WhatsappConnectionAPI*)waAPI)->sendGroupChat(std::string(who),std::string(message));
}
int waAPI_sendimage(void * waAPI, const char * who, int w, int h, unsigned int size, const char * fp) {
	((WhatsappConnectionAPI*)waAPI)->sendImage(std::string(who),w,h,size,fp);
}

void waAPI_sendtyping(void * waAPI,const char * who,int typing) {
	((WhatsappConnectionAPI*)waAPI)->notifyTyping(std::string(who),typing);
}

int waAPI_querychat(void * waAPI, char ** who, char **message, char **author, unsigned long * timestamp) {
	std::string f,m,a; unsigned long t;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chat(f,m,a,t) ) {
		*who = g_strdup(f.c_str());
		*message = g_strdup(m.c_str());
		*author = g_strdup(a.c_str());
		*timestamp = t;
		return 1;
	}
	return 0;
}

void waAPI_accountinfo(void * waAPI, unsigned long long *creation, unsigned long long *freeexpires, char ** status) {
	std::string st;
	unsigned long long cr, fe;
	((WhatsappConnectionAPI*)waAPI)->account_info(cr,fe,st);
	*creation = cr;
	*freeexpires = fe;
	*status = g_strdup(st.c_str());
}

int waAPI_querychatimage(void * waAPI, char ** who, char **image, int * imglen, char ** url, unsigned long * timestamp) {
	std::string fr,im,ur; unsigned long t;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatimages(fr,im,ur,t) ) {
		*who = g_strdup(fr.c_str());
		*image = (char*)g_memdup(im.c_str(),im.size());
		*imglen = im.size();
		*url = g_strdup(ur.c_str());
		*timestamp = t;
		return 1;
	}
	return 0;
}

int waAPI_querychatsound(void * waAPI, char ** who, char ** url, unsigned long * timestamp) {
	std::string fr,ur; unsigned long t;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatsounds(fr,ur,t) ) {
		*who = g_strdup(fr.c_str());
		*url = g_strdup(ur.c_str());
		*timestamp = t;
		return 1;
	}
	return 0;
}

int waAPI_querychatlocation(void * waAPI, char ** who, char **image, int * imglen, double * lat, double * lng, unsigned long * timestamp) {
	std::string fr,im; unsigned long t;
	double la,ln;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_chatlocations(fr,la,ln,im,t) ) {
		*who = g_strdup(fr.c_str());
		*lat = la;
		*lng = ln;
		*image = (char*)g_memdup(im.c_str(),im.size());
		*imglen = im.size();
		*timestamp = t;
		return 1;
	}
	return 0;
}

int waAPI_querystatus(void * waAPI, char ** who, int *stat) {
	std::string f; int st;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_status(f,st) ) {
		*who = g_strdup(f.c_str());
		*stat = st;
		return 1;
	}
	return 0;
}
int waAPI_getuserstatus(void * waAPI, const char * who) {
	return ((WhatsappConnectionAPI*)waAPI)->getuserstatus(who);
}
char * waAPI_getuserstatusstring(void * waAPI, const char * who) {
	if (!waAPI) return 0;
	std::string s = ((WhatsappConnectionAPI*)waAPI)->getuserstatusstring(who);
	return g_strdup(s.c_str());
}
unsigned long long waAPI_getlastseen(void * waAPI, const char * who) {
	return ((WhatsappConnectionAPI*)waAPI)->getlastseen(who);
}

int waAPI_queryicon(void * waAPI, char ** who, char ** icon, int * len, char ** hash) {
	std::string f,ic,hs;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_icon(f,ic,hs) ) {
		*who = g_strdup(f.c_str());
		*icon = (char*)g_memdup(ic.c_str(),ic.size());
		*len = ic.size();
		*hash = g_strdup(hs.c_str());
		return 1;
	}
	return 0;	
}

int waAPI_querytyping(void * waAPI, char ** who, int * stat) {
	std::string f; int status;
	if ( ((WhatsappConnectionAPI*)waAPI)->query_typing(f,status) ) {
		*who = g_strdup(f.c_str());
		*stat = status;
		return 1;
	}
	return 0;	
}

int waAPI_loginstatus(void * waAPI) {
	return ((WhatsappConnectionAPI*)waAPI)->loginStatus();
}

void waAPI_addcontact(void * waAPI, const char * phone) {
	std::vector <std::string> clist;
	clist.push_back(std::string(phone));
	((WhatsappConnectionAPI*)waAPI)->addContacts(clist);
}

void waAPI_delcontact(void * waAPI, const char * phone) {
	
}

void waAPI_setmypresence(void * waAPI, const char * st, const char * msg) {
	((WhatsappConnectionAPI*)waAPI)->setMyPresence(st,msg);
}

// Implementations when Openssl is not present

unsigned char * MD5(const unsigned char *d, int n, unsigned char *md) {
	PurpleCipher *md5_cipher;
	PurpleCipherContext *md5_ctx;
	
	md5_cipher = purple_ciphers_find_cipher("md5");
	md5_ctx = purple_cipher_context_new(md5_cipher, NULL);
	purple_cipher_context_append(md5_ctx, (guchar *)d, n);
	purple_cipher_context_digest(md5_ctx, 16, md, NULL);
	return md;
}

const char hmap[16]  = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
std::string tohex(const char * t, int l) {
	std::string ret;
	for (int i = 0; i < l; i++) {
		ret += hmap[((*t  )>>4)&0xF];
		ret += hmap[((*t++)   )&0xF];
	}
	return ret;
}

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode_esp(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    for(j = i; j < 3; j++)
      ret += "=";

  }

  return ret;

}


std::string SHA256_file_b64(const char *filename) {
	unsigned char md[32];

	PurpleCipher *sha_cipher;
	PurpleCipherContext *sha_ctx;
	
	sha_cipher = purple_ciphers_find_cipher("sha256");
	sha_ctx = purple_cipher_context_new(sha_cipher, NULL);

	FILE * fd = fopen(filename, "rb");
	int read = 0;
	do {
		char buf[1024];
		read = fread(buf,1,1024,fd);
		purple_cipher_context_append(sha_ctx, (guchar *)buf, read);
	} while (read > 0);
	fclose(fd);

	purple_cipher_context_digest(sha_ctx, 32, md, NULL);
	return base64_encode_esp(md,32);
}


std::string md5hex(std::string target) {
	char outh[16];
	MD5((unsigned char*)target.c_str(), target.size(), (unsigned char*)outh);
	return tohex(outh,16);
}

std::string md5raw(std::string target) {
	char outh[16];
	MD5((unsigned char*)target.c_str(), target.size(), (unsigned char*)outh);
	return std::string(outh,16);
}

unsigned char * SHA1(const unsigned char *d, int n, unsigned char *md) {
	PurpleCipher *sha1_cipher;
	PurpleCipherContext *sha1_ctx;
	
	sha1_cipher = purple_ciphers_find_cipher("sha1");
	sha1_ctx = purple_cipher_context_new(sha1_cipher, NULL);
	purple_cipher_context_append(sha1_ctx, (guchar *)d, n);
	purple_cipher_context_digest(sha1_ctx, 20, md, NULL);
	return md;
}

int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter,
                           int keylen, unsigned char *out) {
	unsigned char digtmp[20], *p, itmp[4];
	int cplen, j, k, tkeylen;
	int mdlen = 20;  // SHA1
	unsigned long i = 1;
	
	PurpleCipherContext *context = purple_cipher_context_new_by_name("hmac", NULL);
	
	p = out;
	tkeylen = keylen;
	while(tkeylen)
		{
		if(tkeylen > mdlen)
			cplen = mdlen;
		else
			cplen = tkeylen;
		/* We are unlikely to ever use more than 256 blocks (5120 bits!)
		 * but just in case...
		 */
		itmp[0] = (unsigned char)((i >> 24) & 0xff);
		itmp[1] = (unsigned char)((i >> 16) & 0xff);
		itmp[2] = (unsigned char)((i >> 8) & 0xff);
		itmp[3] = (unsigned char)(i & 0xff);
			
		purple_cipher_context_reset(context, NULL);
		purple_cipher_context_set_option(context, "hash", (gpointer)"sha1");
		purple_cipher_context_set_key_with_len(context, (guchar *)pass, passlen);
		purple_cipher_context_append(context, (guchar *)salt, saltlen);
		purple_cipher_context_append(context, (guchar *)itmp, 4);
		purple_cipher_context_digest(context, mdlen, digtmp, NULL);
		
		memcpy(p, digtmp, cplen);
		for(j = 1; j < iter; j++)
			{
			
			purple_cipher_context_reset(context, NULL);
			purple_cipher_context_set_option(context, "hash", (gpointer)"sha1");
			purple_cipher_context_set_key_with_len(context, (guchar *)pass, passlen);
			purple_cipher_context_append(context, (guchar *)digtmp, mdlen);
			purple_cipher_context_digest(context, mdlen, digtmp, NULL);

			for(k = 0; k < cplen; k++)
				p[k] ^= digtmp[k];
			}
		tkeylen-= cplen;
		i++;
		p+= cplen;
		}
	
	purple_cipher_context_destroy(context);
	
	return 1;
}


// MIME type, copied from mxit
#define		MIME_TYPE_OCTETSTREAM		"application/octet-stream"
#define		ARRAY_SIZE( x )				( sizeof( x ) / sizeof( x[0] ) )

/* supported file mime types */
static struct mime_type {
	const char*		magic;
	const short		magic_len;
	const char*		mime;
} const mime_types[] = {
					/*	magic									length	mime					*/
	/* images */	{	"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",		8,		"image/png"				},		/* image png */
					{	"\xFF\xD8",								2,		"image/jpeg"			},		/* image jpeg */
					{	"\x3C\x3F\x78\x6D\x6C",					5,		"image/svg+xml"			},		/* image SVGansi */
					{	"\xEF\xBB\xBF",							3,		"image/svg+xml"			},		/* image SVGutf */
					{	"\xEF\xBB\xBF",							3,		"image/svg+xml"			},		/* image SVGZ */
	/* mxit */		{	"\x4d\x58\x4d",							3,		"application/mxit-msgs"	},		/* mxit message */
					{	"\x4d\x58\x44\x01",						4,		"application/mxit-mood" },		/* mxit mood */
					{	"\x4d\x58\x45\x01",						4,		"application/mxit-emo"	},		/* mxit emoticon */
					{	"\x4d\x58\x46\x01",						4,		"application/mxit-emof"	},		/* mxit emoticon frame */
					{	"\x4d\x58\x53\x01",						4,		"application/mxit-skin"	},		/* mxit skin */
	/* audio */		{	"\x4d\x54\x68\x64",						4,		"audio/midi"			},		/* audio midi */
					{	"\x52\x49\x46\x46",						4,		"audio/wav"				},		/* audio wav */
					{	"\xFF\xF1",								2,		"audio/aac"				},		/* audio aac1 */
					{	"\xFF\xF9",								2,		"audio/aac"				},		/* audio aac2 */
					{	"\xFF",									1,		"audio/mp3"				},		/* audio mp3 */
					{	"\x23\x21\x41\x4D\x52\x0A",				6,		"audio/amr"				},		/* audio AMR */
					{	"\x23\x21\x41\x4D\x52\x2D\x57\x42",		8,		"audio/amr-wb"			},		/* audio AMR WB */
					{	"\x00\x00\x00",							3,		"audio/mp4"				},		/* audio mp4 */
					{	"\x2E\x73\x6E\x64",						4,		"audio/au"				}		/* audio AU */
};


const char* file_mime_type( const char* filename, const char* buf, int buflen ) {
	unsigned int	i;

	/* check for matching magic headers */
	for ( i = 0; i < ARRAY_SIZE( mime_types ); i++ ) {

		if ( buflen < mime_types[i].magic_len )	/* data is shorter than size of magic */
			continue;

		if ( memcmp( buf, mime_types[i].magic, mime_types[i].magic_len ) == 0 )
			return mime_types[i].mime;
	}

	/* we did not find the MIME type, so return the default (application/octet-stream) */
	return MIME_TYPE_OCTETSTREAM;
}



