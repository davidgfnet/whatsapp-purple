
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
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

std::string md5hex(std::string target);
std::string md5raw(std::string target);

class RC4Decoder; class DataBuffer; class Tree; class WhatsappConnection;

#define MESSAGE_CHAT     0
#define MESSAGE_IMAGE    1
#define MESSAGE_LOCATION 2
enum SessionStatus { SessionNone=0, SessionConnecting=1, SessionWaitingChallenge=2, SessionWaitingAuthOK=3, SessionConnected=4 };
enum ErrorCode { errorAuth, errorUnknown };

std::string base64_decode(std::string const& encoded_string);
unsigned long long str2lng(std::string s);
std::string int2str(unsigned int num);
int str2int(std::string s);
double str2dbl(std::string s);
unsigned char lookupDecoded(std::string value);
std::string getDecoded(int n);

unsigned long long str2lng(std::string s) {
	unsigned long long r;
	sscanf(s.c_str(), "%llu", &r);
	return r;
}
std::string int2str(unsigned int num) {
	char temp[512];
	sprintf(temp,"%d",num);
	return std::string (temp);
}
int str2int(std::string s) {
	int d;
	sscanf(s.c_str(),"%d",&d);
	return d;
}
double str2dbl(std::string s) {
	double d;
	sscanf(s.c_str(),"%lf",&d);
	return d;
}
std::string getusername(std::string user) {
	int pos = user.find('@');
	if (pos != std::string::npos)
		return user.substr(0,pos);
	else
		return user;
}


inline std::map < std::string, std::string > makeAttr1 (std::string k1, std::string v1) {
	std::map < std::string, std::string > at;
	at[k1] = v1;
	return at;
}
inline std::map < std::string, std::string > makeAttr2 (std::string k1, std::string v1, std::string k2, std::string v2) {
	std::map < std::string, std::string > at;
	at[k1] = v1;
	at[k2] = v2;
	return at;
}
inline std::map < std::string, std::string > makeAttr3 (std::string k1, std::string v1, std::string k2, std::string v2, std::string k3, std::string v3) {
	std::map < std::string, std::string > at;
	at[k1] = v1;
	at[k2] = v2;
	at[k3] = v3;
	return at;
}


const char dictionary[256][40] = { "","","","","",  "account","ack","action","active","add","after",
	"ib","all","allow","apple","audio","auth","author","available","bad-protocol","bad-request",
	"before","Bell.caf","body","Boing.caf","cancel","category","challenge","chat","clean","code",
	"composing","config","conflict","contacts","count","create","creation","default","delay",
	"delete","delivered","deny","digest","DIGEST-MD5-1","DIGEST-MD5-2","dirty","elapsed","broadcast",
	"enable","encoding","duplicate","error","event","expiration","expired","fail","failure","false",
	"favorites","feature","features","field","first","free","from","g.us","get","Glass.caf","google",
	"group","groups","g_notify","g_sound","Harp.caf","http://etherx.jabber.org/streams",
	"http://jabber.org/protocol/chatstates","id","image","img","inactive","index","internal-server-error",
	"invalid-mechanism","ip","iq","item","item-not-found","user-not-found","jabber:iq:last","jabber:iq:privacy",
	"jabber:x:delay","jabber:x:event","jid","jid-malformed","kind","last","latitude","lc","leave","leave-all",
	"lg","list","location","longitude","max","max_groups","max_participants","max_subject","mechanism",
	"media","message","message_acks","method","microsoft","missing","modify","mute","name","nokia","none",
	"not-acceptable","not-allowed","not-authorized","notification","notify","off","offline","order","owner",
	"owning","paid","participant","participants","participating","password","paused","picture","pin","ping",
	"platform","pop_mean_time","pop_plus_minus","port","presence","preview","probe","proceed","prop","props",
	"p_o","p_t","query","raw","reason","receipt","receipt_acks","received","registration","relay",
	"remote-server-timeout","remove","Replaced by new connection","request","required","resource",
	"resource-constraint","response","result","retry","rim","s.whatsapp.net","s.us","seconds","server",
	"server-error","service-unavailable","set","show","sid","silent","sound","stamp","unsubscribe","stat",
	"status","stream:error","stream:features","subject","subscribe","success","sync","system-shutdown",
	"s_o","s_t","t","text","timeout","TimePassing.caf","timestamp","to","Tri-tone.caf","true","type",
	"unavailable","uri","url","urn:ietf:params:xml:ns:xmpp-sasl","urn:ietf:params:xml:ns:xmpp-stanzas",
	"urn:ietf:params:xml:ns:xmpp-streams","urn:xmpp:delay","urn:xmpp:ping","urn:xmpp:receipts",
	"urn:xmpp:whatsapp","urn:xmpp:whatsapp:account","urn:xmpp:whatsapp:dirty","urn:xmpp:whatsapp:mms",
	"urn:xmpp:whatsapp:push","user","username","value","vcard","version","video","w","w:g","w:p","w:p:r",
	"w:profile:picture","wait","x","xml-not-well-formed","xmlns","xmlns:stream","Xylophone.caf","1","WAUTH-1",
	"","","","","","","","","","","","XXX","","","","","","",""
};
std::string getDecoded(int n) {
	return std::string(dictionary[n&255]);
}
unsigned char lookupDecoded(std::string value) {
	for (int i = 0; i < 256; i++) {
		if (strcmp(dictionary[i],value.c_str()) == 0)
			return i;
	}
	return 0;
}

const char hexmap[16]  = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };
const char hexmap2[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
class KeyGenerator {
public:
	static void generateKeyImei(const char * imei, const char * salt, int saltlen, char * out) {
		char imeir[strlen(imei)];
		for (int i = 0; i < strlen(imei); i++)
			imeir[i] = imei[strlen(imei)-i-1];
		
		char hash[16];
		MD5((unsigned char*)imeir,strlen(imei),(unsigned char*)hash);
		
		// Convert to hex
		char hashhex[32];
		for (int i = 0; i < 16; i++) {
			hashhex[2*i] = hexmap[(hash[i]>>4)&0xF];
			hashhex[2*i+1] = hexmap[hash[i]&0xF];
		}
		
		PKCS5_PBKDF2_HMAC_SHA1 (hashhex,32,(unsigned char*)salt,saltlen,16,20,(unsigned char*)out);
	}
	static void generateKeyV2(const std::string pw, const char * salt, int saltlen, char * out) {
		std::string dec = base64_decode(pw);
		
		PKCS5_PBKDF2_HMAC_SHA1 (dec.c_str(),20,(unsigned char*)salt,saltlen,16,20,(unsigned char*)out);
	}
	static void generateKeyMAC(std::string macaddr, const char * salt, int saltlen, char * out) {
		macaddr = macaddr+macaddr;
		
		char hash[16];
		MD5((unsigned char*)macaddr.c_str(),34,(unsigned char*)hash);
		
		// Convert to hex
		char hashhex[32];
		for (int i = 0; i < 16; i++) {
			hashhex[2*i] = hexmap[(hash[i]>>4)&0xF];
			hashhex[2*i+1] = hexmap[hash[i]&0xF];
		}
		
		PKCS5_PBKDF2_HMAC_SHA1 (hashhex,32,(unsigned char*)salt,saltlen,16,20,(unsigned char*)out);
	}
	static void calc_hmac(const unsigned char *data, int l, const unsigned char *key, unsigned char * hmac) {
		unsigned char temp[20];
		HMAC_SHA1 (data,l,key,20,temp);
		memcpy(hmac,temp,4);
	}
private:
	static void HMAC_SHA1(const unsigned char *text,int text_len,const unsigned char *key, int key_len,unsigned char *digest) {
		unsigned char SHA1_Key[4096], AppendBuf2[4096], szReport[4096];
		unsigned char * AppendBuf1 = new unsigned char[text_len+64];
		unsigned char m_ipad[64], m_opad[64];

		memset(SHA1_Key, 0, 64);
		memset(m_ipad, 0x36, sizeof(m_ipad));
		memset(m_opad, 0x5c, sizeof(m_opad));

		if (key_len > 64)
			SHA1(key,key_len,SHA1_Key);
		else
			memcpy(SHA1_Key, key, key_len);

		for (int i=0; i<sizeof(m_ipad); i++)
			m_ipad[i] ^= SHA1_Key[i];              

		memcpy(AppendBuf1, m_ipad, sizeof(m_ipad));
		memcpy(AppendBuf1 + sizeof(m_ipad), text, text_len);

		SHA1(AppendBuf1, sizeof(m_ipad) + text_len, szReport);

		for (int j=0; j<sizeof(m_opad); j++)
			m_opad[j] ^= SHA1_Key[j];

		memcpy(AppendBuf2, m_opad, sizeof(m_opad));
		memcpy(AppendBuf2 + sizeof(m_opad), szReport, 20);

		SHA1(AppendBuf2, sizeof(m_opad) + 20, digest);
	
		delete [] AppendBuf1;
	}
};

class RC4Decoder {
public:
	unsigned char s[256];
	unsigned char i,j;
	inline void swap (unsigned char i, unsigned char j) {
		unsigned char t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
public:
	RC4Decoder(const unsigned char * key, int keylen, int drop) {
		for (unsigned int k = 0; k < 256; k++) s[k] = k;
		i = j = 0;
		do {
			unsigned char k = key[i % keylen];
			j = (j + k + s[i]) & 0xFF;
			swap(i,j);
		} while (++i != 0);
		i = j = 0;
		
		unsigned char temp[drop];
		for (int k = 0; k < drop; k++) temp[k] = k;
		cipher(temp,drop);
	}
	
	void cipher (unsigned char * data, int len) {
		while (len--) {
			i++;
			j += s[i];
			swap(i,j);
			unsigned char idx = s[i]+s[j];
			*data++ ^= s[idx];
		}
	}
};

class DataBuffer {
private:
	unsigned char * buffer;
	int blen;
public:
	DataBuffer (const void * ptr = 0, int size = 0) {
		if (ptr != NULL and size > 0) {
			buffer = (unsigned char*)malloc(size+1);
			memcpy(buffer,ptr,size);
			blen = size;
		}else{
			blen = 0;
			buffer = (unsigned char*)malloc(1024);
		}
	}
	~DataBuffer() {
		free(buffer);
	}
	DataBuffer (const DataBuffer & other) {
		blen = other.blen;
		buffer = (unsigned char*)malloc(blen+1024);
		memcpy(buffer,other.buffer,blen);
	}
	DataBuffer operator+(const DataBuffer & other) const {
		DataBuffer result = *this;
		
		result.addData(other.buffer,other.blen);
		return result;
	}
	DataBuffer & operator = (const DataBuffer & other) {
		if (this != &other) {
			free(buffer);
			this->blen = other.blen;
			buffer = (unsigned char*)malloc(blen+1024);
			memcpy(buffer,other.buffer,blen);
		}
		return *this;
	}
	DataBuffer (const DataBuffer * d) {
		blen = d->blen;
		buffer = (unsigned char*)malloc(blen+1024);
		memcpy(buffer,d->buffer,blen);
	}
	DataBuffer * decodedBuffer(RC4Decoder * decoder, int clength, bool dout) {
		DataBuffer * deco = new DataBuffer(this->buffer,clength);
		if (dout) decoder->cipher(&deco->buffer[0],clength-4);
		else      decoder->cipher(&deco->buffer[4],clength-4);
		return deco;
	}
	DataBuffer encodedBuffer(RC4Decoder * decoder, unsigned char* key, bool dout) {
		DataBuffer deco = *this;
		decoder->cipher(&deco.buffer[0],blen);
		unsigned char hmacint[4]; DataBuffer hmac;
		KeyGenerator::calc_hmac(deco.buffer,blen,key,(unsigned char*)&hmacint);
		hmac.addData(hmacint,4);
		
		if (dout) deco = deco + hmac;
		else      deco = hmac + deco;
		
		return deco;
	}
	void clear() {
		blen = 0;
		free(buffer);
		buffer = (unsigned char*)malloc(1);
	}
	void * getPtr() { return buffer; }
	void addData(const void * ptr, int size) {
		if (ptr != NULL and size > 0) {
			buffer = (unsigned char*)realloc(buffer,blen+size);
			memcpy(&buffer[blen],ptr,size);
			blen += size;
		}
	}
	void popData(int size) {
		if (size > blen) {
			throw 0;
		}else{
			memmove(&buffer[0],&buffer[size],blen-size);
			blen -= size;
			if (blen+size > blen*2 and blen > 8*1024)
				buffer = (unsigned char*)realloc(buffer,blen+1);
		}
	}
	void crunchData(int size) {
		if (size > blen) {
			throw 0;
		}else{
			blen -= size;
		}
	}
	int getInt(int nbytes, int offset = 0) {
		if (nbytes > blen)
			throw 0;
		int ret = 0;
		for (int i = 0; i < nbytes; i++) {
			ret <<= 8;
			ret |= buffer[i+offset];
		}
		return ret;
	}
	void putInt(int value, int nbytes) {
		assert(nbytes > 0);
		
		unsigned char out[nbytes];
		for (int i = 0; i < nbytes; i++) {
			out[nbytes-i-1] = (value>>(i<<3))&0xFF;
		}
		this->addData(out,nbytes);
	}
	int readInt(int nbytes) {
		if (nbytes > blen)
			throw 0;
		int ret = getInt(nbytes);
		popData(nbytes);
		return ret;
	}
	int readListSize() {
		if (blen == 0)
			throw 0;
		int ret;
		if (buffer[0] == 0xf8 or buffer[0] == 0xf3) {
			ret = buffer[1];
			popData(2);
		}
		else if (buffer[0] == 0xf9) {
			ret = getInt(2,1);
			popData(3);
		}
		else {
			// FIXME throw 0 error
			ret = -1;
			printf("Parse error!!\n");
		}
		return ret;
	}
	void writeListSize(int size) {
		if (size == 0) {
			putInt(0,1);
		}
		else if (size < 256) {
			putInt(0xf8,1);
			putInt(size,1);
		}
		else {
			putInt(0xf9,1);
			putInt(size,2);
		}
	}
	std::string readRawString(int size) {
		if (size < 0 or size > blen)
			throw 0;
		std::string st(size,' ');
		memcpy(&st[0],buffer,size);
		popData(size);
		return st;
	}
	std::string readString() {
		if (blen == 0)
			throw 0;
		int type = readInt(1);
		if (type > 4 and type < 0xf5) {
			return getDecoded(type);
		}
		else if (type == 0) {
			return "";
		}
		else if (type == 0xfc) {
			int slen = readInt(1);
			return readRawString(slen);
		}
		else if (type == 0xfd) {
			int slen = readInt(3);
			return readRawString(slen);
		}
		else if (type == 0xfe) {
			return getDecoded(readInt(1)+0xf5);
		}
		else if (type == 0xfa) {
			std::string u = readString();
			std::string s = readString();
			
			if (u.size() > 0 and s.size() > 0)
				return u + "@" + s;
			else if (s.size() > 0)
				return s;
			return "";
		}
		return "";
	}
	void putRawString(std::string s) {
		if (s.size() < 256) {
			putInt(0xfc,1);
			putInt(s.size(),1);
			addData(s.c_str(),s.size());
		}
		else {
			putInt(0xfd,1);
			putInt(s.size(),3);
			addData(s.c_str(),s.size());
		}
	}
	void putString(std::string s) {
		unsigned char lu = lookupDecoded(s);
		if (lu > 4 and lu < 0xf5) {
			putInt(lu,1);
		}
		else if (s.find('@') != std::string::npos) {
			std::string p1 = s.substr(0,s.find('@'));
			std::string p2 = s.substr(s.find('@')+1);
			putInt(0xfa,1);
			putString(p1);
			putString(p2);
		}
		else if (s.size() < 256) {
			putInt(0xfc,1);
			putInt(s.size(),1);
			addData(s.c_str(),s.size());
		}
		else {
			putInt(0xfd,1);
			putInt(s.size(),3);
			addData(s.c_str(),s.size());
		}
	}
	bool isList() {
		if (blen == 0)
			throw 0;
		return (buffer[0] == 248 or buffer[0] == 0 or buffer[0] == 249);
	}
	std::vector <Tree> readList(WhatsappConnection * c);
	int size() { return blen; }
	std::string toString() {
		std::string r(blen,' ');
		memcpy(&r[0],buffer,blen);
		return r;
	}
};

class Tree {
private:
	std::map < std::string, std::string > attributes;
	std::vector < Tree > children;
	std::string tag, data;
	bool forcedata;
public:
	Tree(std::string tag = "") {this->tag = tag; forcedata=false;}
	Tree(std::string tag, std::map < std::string, std::string > attributes) {
		this->tag = tag;
		this->attributes = attributes;
		forcedata = false;
	}
	~Tree() {
	}
	void forceDataWrite() {forcedata=true;}
	bool forcedData() const { return forcedata; }
	void addChild(Tree t) {
		children.push_back(t);
	}
	void setTag(std::string tag) {
		this->tag = tag;
	}
	void setAttributes(std::map < std::string, std::string > attributes) {
		this->attributes = attributes;
	}
	void readAttributes(DataBuffer * data, int size) {
		int count = (size - 2 + (size % 2)) / 2;
		while (count--) {
			std::string key = data->readString();
			std::string value = data->readString();
			attributes[key] = value;
		}
	}
	void writeAttributes(DataBuffer * data) {
		for (std::map<std::string, std::string>::iterator iter = attributes.begin();  iter != attributes.end(); iter++) {
			data->putString(iter->first);
			data->putString(iter->second);
		}
	}
	void setData(const std::string d) {
		data = d;
	}
	std::string getData() { return data; }
	std::string getTag()  { return tag; }
	void setChildren(std::vector < Tree > c) {
		children = c;
	}
	std::vector < Tree > getChildren() {
		return children;
	}
	std::map < std::string, std::string > & getAttributes() {
		return attributes;
	}
	bool hasAttributeValue(std::string at, std::string val) {
		if (hasAttribute(at)) {
			return (attributes[at] == val);
		}
		return false;
	}
	bool hasAttribute(std::string at) {
		return (attributes.find(at) != attributes.end());
	}
	std::string getAttribute(std::string at) {
		if (hasAttribute(at))
			return (attributes[at]);
		return "";
	}
	
	Tree getChild(std::string tag) {
		for (unsigned int i = 0; i < children.size(); i++) {
			if (children[i].getTag() == tag)
				return children[i];
			Tree t = children[i].getChild(tag);
			if (t.getTag() != "treeerr")
				return t;
		}
		return Tree("treeerr");
	}
	bool hasChild(std::string tag) {
		for (unsigned int i = 0; i < children.size(); i++) {
			if (children[i].getTag() == tag)
				return true;
			if (children[i].hasChild(tag))
				return true;
		}
		return false;
	}
	
	std::string toString(int sp = 0) {
		std::string ret;
		std::string spacing(' ',sp);
		ret += spacing+"Tag: "+tag+"\n";
		for (std::map<std::string, std::string>::iterator iter = attributes.begin();  iter != attributes.end(); iter++) {
			ret += spacing+"at["+iter->first+"]="+iter->second+"\n";
		}
		ret += spacing+"Data: "+data+"\n";
		
		for (unsigned int i = 0; i < children.size(); i++) {
			ret += children[i].toString(sp+1);
		}
		return ret;
	}
};


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

class Contact {
public:
	Contact() {}
	Contact(std::string phone, bool myc) {
		this->phone = phone;
		this->mycontact = myc;
		this->last_seen = 0;
		this->subscribed = false;
		this->typing = "paused";
		this->status = "";
	}
	
	std::string phone, name;
	std::string presence, typing;
	std::string status;
	unsigned long long last_seen, last_status;
	bool mycontact;
	std::string ppprev, pppicture;
	bool subscribed;
};

class Message {
public:
	Message(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id, const std::string author) {
		int pos = from.find('@');
		if (pos != std::string::npos) {
			this->from = from.substr(0,pos);
			this->server = from.substr(pos+1);
		}
		else
			this->from = from;
		this->t = time;
		this->wc = const_cast <WhatsappConnection*> (wc);
		this->id = id;
		this->author = getusername(author);
	}
	virtual ~Message() {}
	std::string from, server, author;
	unsigned long long t;
	std::string id;
	WhatsappConnection * wc;
	
	virtual int type() const = 0;
	
	virtual Message * copy() const = 0;
};

class WhatsappConnection {
friend class ChatMessage;
friend class ImageMessage;
friend class Message;
private:
	// Current dissection classes
	RC4Decoder * in, * out;
	unsigned char session_key[20];
	DataBuffer inbuffer, outbuffer;
	DataBuffer sslbuffer, sslbuffer_in;
	std::string challenge_data, challenge_response;
	std::string phone,password;
	SessionStatus conn_status;
	
	// State stuff
	unsigned int msgcounter, iqid;
	std::string nickname;
	std::string whatsappserver,whatsappservergroup;
	std::string mypresence, mymessage;
	
	// Various account info
	std::string account_type, account_status, account_expiration, account_creation;
	
	// Groups stuff
	std::map <std::string,Group> groups;
	int gq_stat;
	int gw1,gw2,gw3;
	bool groups_updated;
	
	// Contacts & msg
	std::map < std::string, Contact > contacts;
	std::vector <Message*> recv_messages, recv_messages_delay;
	std::vector <std::string> user_changes, user_icons, user_typing;
	
	void processIncomingData();
	void processSSLIncomingData();
	DataBuffer serialize_tree(Tree * tree, bool crypt);
	DataBuffer write_tree(Tree * tree);
	Tree parse_tree(DataBuffer * data);
	
	// HTTP interface
	std::string generateHttpAuth(std::string nonce);

	// SSL / HTTPS interface
	std::string sslnonce;
	int sslstatus;  // 0 none, 1/2 requesting A, 3/4 requesting Q

	void receiveMessage(const Message & m);
	void notifyPresence(std::string from, std::string presence);
	void notifyLastSeen(std::string from, std::string seconds);
	void addPreviewPicture(std::string from, std::string picture);
	void addFullsizePicture(std::string from, std::string picture);
	void sendResponse();
	void doPong(std::string id, std::string from);
	void subscribePresence(std::string user);
	void getLast(std::string user);
	void queryPreview(std::string user);
	void gotTyping(std::string who, std::string tstat);
	void updateGroups();
	
	void notifyMyMessage();
	void notifyMyPresence();
	void sendInitial();
	void notifyError(ErrorCode err);
	DataBuffer generateResponse(std::string from,std::string type,std::string id);

	void generateSyncARequest();
	void generateSyncQRequest();
	void updateContactStatuses(std::string json);

public:
	Tree read_tree(DataBuffer * data);

	WhatsappConnection(std::string phone, std::string password, std::string nick);
	~WhatsappConnection();
	
	void doLogin(std::string );
	void receiveCallback(const char * data, int len);
	int  sendCallback(char * data, int len);
	void sentCallback(int len);
	bool hasDataToSend();
	
	void addContacts(std::vector <std::string> clist);
	void sendChat(std::string to, std::string message);
	void sendGroupChat(std::string to, std::string message);
	bool query_chat(std::string & from, std::string & message,std::string & author, unsigned long & t);
	bool query_chatimages(std::string & from, std::string & preview, std::string & url, unsigned long & t);
	bool query_chatsounds(std::string & from, std::string & url, unsigned long & t);
	bool query_chatlocations(std::string & from, double & lat, double & lng, std::string & prev, unsigned long & t);
	bool query_status(std::string & from, int & status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	bool query_typing(std::string & from, int & status);
	void send_avatar(const std::string & avatar);
	void account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);
	
	void manageParticipant(std::string group, std::string participant, std::string command);
	void leaveGroup(std::string group);
	
	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);
	std::map <std::string,Group> getGroups();
	bool groupsUpdated();
	void addGroup(std::string subject);
		
	int loginStatus() const {
		return ((int)conn_status)-1;
	}

	int sendSSLCallback(char* buffer, int maxbytes);
	int sentSSLCallback(int bytessent);
	void receiveSSLCallback(char* buffer, int bytesrecv);
	bool hasSSLDataToSend();
	void SSLCloseCallback();
	bool hasSSLConnection(std::string & host, int * port);

	std::string generateHeaders(std::string auth, int content_length);
};

class ChatMessage: public Message{
public:
	ChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id, const std::string message, const std::string author) :
		Message(wc, from, time, id, author) {
		
		this->message = message;
	}
	int type() const { return 0; }
	
	std::string message; // Message
	
	DataBuffer serialize() const {
		Tree request("request",makeAttr1("xmlns","urn:xmpp:receipts"));
		Tree notify ("notify", makeAttr2("xmlns","urn:xmpp:whatsapp", "name",from));
		Tree xhash  ("x",      makeAttr1("xmlns","jabber:x:event"));
		xhash.addChild(Tree("server"));
		Tree tbody("body"); tbody.setData(this->message);
		
		std::string stime = int2str(t);
		std::map <std::string, std::string> attrs;
		if (server.size())
			attrs["to"] = from+"@"+server;
		else
			attrs["to"] = from+"@"+wc->whatsappserver;
		attrs["type"] = "chat";
		attrs["id"] = stime+"-"+id;
		attrs["t"]  = stime;
		
		Tree mes("message",attrs);
		mes.addChild(xhash); mes.addChild(notify);
		mes.addChild(request); mes.addChild(tbody);
		
		return wc->serialize_tree(&mes,true);
	}
	
	Message * copy () const {
		return new ChatMessage(wc,from,t,id,message,author);
	}
};


class ImageMessage: public Message{
public:
	ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id,
				const std::string author, const std::string url, const unsigned int width, 
				const unsigned int height, const unsigned int size, const std::string encoding,
				const std::string hash, const std::string filetype, const std::string preview) :
		Message(wc, from, time, id, author) {
		
		this->url = url;
		this->width = width;
		this->height = height;
		this->size = size;
		this->encoding = encoding;
		this->hash = hash;
		this->filetype = filetype;
		this->preview = preview;
	}
	int type() const { return 1; }
	
	Message * copy () const {
		return new ImageMessage(wc,from,t,id,author,url,width,height,size,encoding,hash,filetype,preview);
	}
	
	std::string url; // Image URL
	std::string encoding, hash, filetype;
	std::string preview;
	unsigned int width, height, size;
};


class SoundMessage: public Message{
public:
	SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id,
				const std::string author, const std::string url, const std::string hash,
				const std::string filetype) :
		Message(wc, from, time, id, author) {
		
		this->url = url;
		this->filetype = filetype;
	}
	int type() const { return 3; }
	
	Message * copy () const {
		return new SoundMessage(wc,from,t,id,author,url,hash,filetype);
	}
	
	std::string url; // Sound URL
	std::string hash, filetype;
};


class LocationMessage: public Message {
public:
	LocationMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id, 
		const std::string author, double lat, double lng, std::string preview) :
		Message(wc, from, time, id, author) {
		
		this->latitude  = lat;
		this->longitude = lng;
		this->preview = preview;
	}
	int type() const { return 2; }
	
	Message * copy () const {
		return new LocationMessage(wc,from,t,id,author,latitude,longitude,preview);
	}
	
	double latitude, longitude; // Location
	std::string preview;
};

DataBuffer WhatsappConnection::generateResponse(std::string from,std::string type,std::string id) {
	Tree received("received",makeAttr1("xmlns","urn:xmpp:receipts"));

	//std::string stime = int2str(t);
	std::map <std::string, std::string> attrs;
	attrs["to"] = from;
	attrs["type"] = type;
	attrs["id"] = id;

	Tree mes("message",attrs);
	mes.addChild(received);

	return serialize_tree(&mes,true);
}

std::vector <Tree> DataBuffer::readList(WhatsappConnection * c) {
	std::vector <Tree> l;
	int size = readListSize();
	while (size--) {
		l.push_back(c->read_tree(this));
	}
	return l;
}

void WhatsappConnection::generateSyncARequest() {
	sslbuffer.clear();

	std::string httpr = "POST /v2/sync/a HTTP/1.1\r\n" + generateHeaders(generateHttpAuth("0"),0) + "\r\n";

	sslbuffer.addData(httpr.c_str(),httpr.size());
}
void WhatsappConnection::generateSyncQRequest() {
	sslbuffer.clear();

	// Query numbers with and without "+"
	// Seems that american numbers do not like the + symbol
	std::string body = "ut=all&t=c";
	for (std::map<std::string, Contact>::iterator iter = contacts.begin();  iter != contacts.end(); iter++) {
		body += ("&u[]=" + iter->first);
		body += ("&u[]=%2B" + iter->first);
	}
	std::string httpr = "POST /v2/sync/q HTTP/1.1\r\n" + 
		generateHeaders(generateHttpAuth(sslnonce),body.size()) + "\r\n";
	httpr += body;

	sslbuffer.addData(httpr.c_str(),httpr.size());
}


WhatsappConnection::WhatsappConnection(std::string phone, std::string password, std::string nickname) {
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
	
	// Trim password spaces
	while (password.size() > 0 and password[0] == ' ')
		password = password.substr(1);
	while (password.size() > 0 and password[password.size()-1] == ' ')
		password = password.substr(0,password.size()-1);
}

WhatsappConnection::~WhatsappConnection() {
	if (this->in)  delete this->in;
	if (this->out) delete this->out;
	for (int i = 0; i < recv_messages.size(); i++) {
		delete recv_messages[i];
	}
}

std::map <std::string,Group> WhatsappConnection::getGroups() {
	return groups;
}

bool WhatsappConnection::groupsUpdated() {
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

void WhatsappConnection::updateGroups() {
	// Get the group list
	groups.clear();
	{
		gw1 = iqid;
		Tree iq("list", makeAttr2("xmlns","w:g", "type","owning"));
		Tree req("iq", makeAttr3("id",int2str(iqid++), "type","get", "to","g.us"));
		req.addChild(iq);
		outbuffer = outbuffer + serialize_tree(&req,true);
	}
	{
		gw2 = iqid;
		Tree iq("list", makeAttr2("xmlns","w:g", "type","participating"));
		Tree req("iq", makeAttr3("id",int2str(iqid++), "type","get", "to","g.us"));
		req.addChild(iq);
		outbuffer = outbuffer + serialize_tree(&req,true);	
	}
	gq_stat = 1;  // Queried the groups
	gw3 = 0;
}


void WhatsappConnection::manageParticipant(std::string group, std::string participant, std::string command) {
	Tree part("participant", makeAttr1("jid",participant));
	Tree iq(command, makeAttr1("xmlns","w:g"));
	iq.addChild(part);
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","set", "to",group+"@g.us"));
	req.addChild(iq);
	
	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::leaveGroup(std::string group) {
	Tree gr("group", makeAttr1("id",group+"@g.us"));
	Tree iq("leave", makeAttr1("xmlns","w:g"));
	iq.addChild(gr);
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","set", "to","g.us"));
	req.addChild(iq);
	
	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::addGroup(std::string subject) {
	Tree gr("group", makeAttr3("xmlns","w:g","action","create","subject",subject));
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","set", "to","g.us"));
	req.addChild(gr);
	
	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::doLogin(std::string useragent) {
	// Send stream init
	DataBuffer first;
	
	{
	std::map <std::string,std::string> auth;
	first.addData("WA\1\2",4);
	auth["resource"] = useragent;
	auth["to"] = "s.whatsapp.net";
	Tree t("start",auth);
	first = first + serialize_tree(&t,false);
	}

	// Send features
	{
	Tree p;
	p.setTag("stream:features");
	p.addChild(Tree("receipt_acks"));
	p.addChild(Tree("w:profile:picture",makeAttr1("type","all")));
	p.addChild(Tree("w:profile:picture",makeAttr1("type","group")));
	p.addChild(Tree("notification",makeAttr1("type","participant")));
	p.addChild(Tree("status"));
	first = first + serialize_tree(&p,false);
        }
        
	// Send auth request
	{
	std::map <std::string,std::string> auth;
	auth["xmlns"]     = "urn:ietf:params:xml:ns:xmpp-sasl";
	auth["mechanism"] = "WAUTH-1";
	auth["user"]      = phone;
	Tree t("auth",auth);
	t.forceDataWrite();
	first = first + serialize_tree(&t,false);
	}
	
	conn_status = SessionWaitingChallenge;
	outbuffer = first;
}

void WhatsappConnection::receiveCallback(const char * data, int len) {
	if (data != NULL and len > 0)
		inbuffer.addData(data,len);
	this->processIncomingData();
}

int WhatsappConnection::sendCallback(char * data, int len) {
	int minlen = outbuffer.size();
	if (minlen > len) minlen = len;
	
	memcpy(data,outbuffer.getPtr(),minlen);
	return minlen;
}

bool WhatsappConnection::hasDataToSend() {
	return outbuffer.size() != 0;
}

void WhatsappConnection::sentCallback(int len) {
	outbuffer.popData(len);
}


int WhatsappConnection::sendSSLCallback(char* buffer, int maxbytes) {
	int minlen = sslbuffer.size();
	if (minlen > maxbytes) minlen = maxbytes;
	
	memcpy(buffer,sslbuffer.getPtr(),minlen);
	return minlen;
}
int WhatsappConnection::sentSSLCallback(int bytessent) {
	sslbuffer.popData(bytessent);
}
void WhatsappConnection::receiveSSLCallback(char* buffer, int bytesrecv) {
	if (buffer != NULL and bytesrecv > 0)
		sslbuffer_in.addData(buffer,bytesrecv);
	this->processSSLIncomingData();
}
bool WhatsappConnection::hasSSLDataToSend() {
	return sslbuffer.size() != 0;
}
void WhatsappConnection::SSLCloseCallback() {
	sslstatus = 0;
}
bool WhatsappConnection::hasSSLConnection(std::string & host, int * port) {
	host = "sro.whatsapp.net";
	*port = 443;
	return (sslstatus == 1 or sslstatus == 3);
}


void WhatsappConnection::subscribePresence(std::string user) {
	Tree request("presence",makeAttr2("type","subscribe", "to",user));
	outbuffer = outbuffer + serialize_tree(&request,true);
}

void WhatsappConnection::getLast(std::string user) {
	Tree iq("query", makeAttr1("xmlns","jabber:iq:last"));
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","get", "to",user));
	req.addChild(iq);

	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::gotTyping(std::string who, std::string tstat) {
	who = getusername(who);
	if (contacts.find(who) != contacts.end()) {
		contacts[who].typing = tstat;
		user_typing.push_back(who);
	}
}

void WhatsappConnection::notifyTyping(std::string who, int status) {
	std::string s = "paused";
	if (status == 1) s = "composing";
	
	Tree mes("message",makeAttr2("to",who+"@"+whatsappserver, "type","chat"));
	mes.addChild(Tree(s,makeAttr1("xmlns","http://jabber.org/protocol/chatstates")));

	outbuffer = outbuffer +serialize_tree(&mes,true);
}

void WhatsappConnection::account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status) {
	creation = str2lng(account_creation);
	freeexp =  str2lng(account_expiration);
	status = account_status;
}

void WhatsappConnection::queryPreview(std::string user) {
	Tree pic("picture", makeAttr2("xmlns","w:profile:picture", "type","preview" ));
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","get", "to",user));
	req.addChild(pic);

	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::send_avatar(const std::string & avatar) {
	Tree pic ("picture", makeAttr2("type","image", "xmlns","w:profile:picture" ));
	Tree prev("picture", makeAttr1("type","preview" ));
	pic.setData(avatar);
	prev.setData(avatar);
	Tree req("iq", makeAttr3("id",int2str(iqid++), "type","set", "to",phone+"@"+whatsappserver));
	req.addChild(pic);
	req.addChild(prev);
	
	outbuffer = outbuffer + serialize_tree(&req,true);
}

void WhatsappConnection::sendChat(std::string to, std::string message) {
	ChatMessage msg(this, to, time(NULL), int2str(msgcounter++), message, "");
	DataBuffer buf =  msg.serialize();

	outbuffer = outbuffer + buf;
}
void WhatsappConnection::sendGroupChat(std::string to, std::string message) {
	ChatMessage msg(this, to, time(NULL), int2str(msgcounter++), message, "");
	msg.server = "g.us";
	DataBuffer buf =  msg.serialize();

	outbuffer = outbuffer + buf;
}

void WhatsappConnection::addContacts(std::vector <std::string> clist) {
	// Insert the contacts to the contact list
	for (unsigned int i = 0; i < clist.size(); i++) {
		if (contacts.find(clist[i]) == contacts.end())
			contacts[clist[i]] = Contact(clist[i],true);
		else
			contacts[clist[i]].mycontact = true;
			
		user_changes.push_back(clist[i]);
	}
	// Query the profile pictures
	for (std::map<std::string, Contact>::iterator iter = contacts.begin();  iter != contacts.end(); iter++) {
		if (not iter->second.subscribed) {
			iter->second.subscribed = true;
			
			this->subscribePresence(iter->first+"@"+whatsappserver);
			this->queryPreview(iter->first+"@"+whatsappserver);
			this->getLast(iter->first+"@"+whatsappserver);
		}
	}
	// Query statuses
	if (sslstatus == 0) {
		sslbuffer_in.clear();
		sslstatus = 1;
		generateSyncARequest();
	}
}

unsigned char hexchars(char c1, char c2) {
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

std::string utf8_decode(std::string in) {
	std::string dec;
	for (int i = 0; i < in.size(); i++) {
		if (in[i] == '\\' and in[i+1] == 'u') {
			i += 2; // Skip \u

			while (i < in.size()) {
				unsigned char hex = hexchars(in[i],in[i+1]);
				dec += (char)hex;
				i += 2;
				if (not (hex >= 0x80 and hex <= 0xBF))
					break;
			}
		}
		else
			dec += in[i];
	}
	return dec;
}

std::string query_field(std::string work, std::string lo, bool integer = false) {
	int p = work.find("\""+lo+"\"");
	if (p == std::string::npos) return "";

	work = work.substr(p+("\""+lo+"\"").size());
	
	p = work.find("\"");
	if (integer) p = work.find(":");
	if (p == std::string::npos) return "";
	
	work = work.substr(p+1);

	p = work.find("\"");
	if (integer) {
		p = 0;
		while (p < work.size() and work[p] >= '0' and work[p] <= '9') p++;
	}
	if (p == std::string::npos) return "";

	work = work.substr(0,p);

	return work;
}

void WhatsappConnection::updateContactStatuses(std::string json) {
	while (true) {
		int offset = json.find("{");
		if (offset == std::string::npos) break;
		json = json.substr(offset+1);

		// Look for closure
		int cl = json.find("{");
		if (cl == std::string::npos) cl = json.size();
		std::string work = json.substr(0,cl);

		// Look for "n", the number and "w","t","s"
		std::string n = query_field(work,"n");
		std::string w = query_field(work,"w",true);
		std::string t = query_field(work,"t",true);
		std::string s = query_field(work,"s");

		if (w == "1") {
			contacts[n].status = utf8_decode(s);
			contacts[n].last_status = str2lng(t);
		}

		json = json.substr(cl);
	}
}

// Quick and dirty way to parse the HTTP responses
void WhatsappConnection::processSSLIncomingData() {
	// Parse HTTPS headers and JSON body
	if (sslstatus == 1 or sslstatus == 3) sslstatus++;

	if (sslstatus == 2) {
		std::string toparse((char*)sslbuffer_in.getPtr(),sslbuffer_in.size());
		if (toparse.find("nonce=\"") != std::string::npos) {
			toparse = toparse.substr(toparse.find("nonce=\"") + 7);
			if (toparse.find("\"") != std::string::npos) {
				toparse = toparse.substr(0,toparse.find("\""));
				sslnonce = toparse;
				sslstatus = 4;

				sslbuffer.clear();
				sslbuffer_in.clear();
				generateSyncQRequest();
			}
		}
	}
	if (sslstatus == 4) {
		// Look for the first line, to be 200 OK
		std::string toparse((char*)sslbuffer_in.getPtr(),sslbuffer_in.size());
		if (toparse.find("\r\n") != std::string::npos) {
			std::string fl = toparse.substr(0,toparse.find("\r\n"));
			if (fl.find("200") == std::string::npos)
				goto abortStatus;

			if (toparse.find("\r\n\r\n") != std::string::npos) {
				std::string headers = toparse.substr(0,toparse.find("\r\n\r\n")+4);
				std::string content = toparse.substr(toparse.find("\r\n\r\n")+4);

				// Look for content length
				if (headers.find("Content-Length:") != std::string::npos) {
					std::string clen = headers.substr(headers.find("Content-Length:")+strlen("Content-Length:"));
					clen = clen.substr(0,clen.find("\r\n"));
					while (clen.size() > 0 and clen[0] == ' ')
						clen = clen.substr(1);
					int contentlength = str2int(clen);
					if (contentlength == content.size()) {
						// Now we can proceed to parse the JSON
						updateContactStatuses(content);
						sslstatus = 0;
					}
				}
			}
		}
	}

	return;
	abortStatus:
	sslstatus = 0;
	return;
}

void WhatsappConnection::processIncomingData() {
	// Parse the data and create as many Trees as possible
	std::vector < Tree> treelist;
	try {
		if (inbuffer.size() >=3) {
			// Consume as many trees as possible
			Tree t;
			do {
				t = parse_tree(&inbuffer);
				if (t.getTag() != "treeerr")
					treelist.push_back(t);
			} while (t.getTag() != "treeerr" and inbuffer.size() >= 3);
		}
	}catch (int n) {
		printf("In stream error! Need to handle this properly...\n");
		return;
	}
	
	// Now process the tree list!
	for (unsigned int i = 0; i < treelist.size(); i++) {
		if (treelist[i].getTag() == "challenge") {
			// Generate a session key using the challege & the password
			assert(conn_status == SessionWaitingChallenge);
			
			if (password.size() == 15) {
				KeyGenerator::generateKeyImei(password.c_str(),
					treelist[i].getData().c_str(),treelist[i].getData().size(),(char*)this->session_key);
			}
			else if (password.find(":") != std::string::npos) {
				KeyGenerator::generateKeyMAC(password,
					treelist[i].getData().c_str(),treelist[i].getData().size(),(char*)this->session_key);
			}
			else {
				KeyGenerator::generateKeyV2(password,
					treelist[i].getData().c_str(),treelist[i].getData().size(),(char*)this->session_key);
			}
			
			in = new RC4Decoder(session_key, 20, 256);
			out = new RC4Decoder(session_key, 20, 256);
			
			conn_status = SessionWaitingAuthOK;
			challenge_data = treelist[i].getData();
			
			this->sendResponse();
		}
		else if (treelist[i].getTag() == "success") {
			// Notifies the success of the auth
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
			this->sendInitial();
			this->updateGroups();
			
			//std::cout << "Logged in!!!" << std::endl;
			//std::cout << "Account " << phone << " status: " << account_status << " kind: " << account_type <<
			//	" expires: " << account_expiration << " creation: " << account_creation << std::endl;
		}
		else if (treelist[i].getTag() == "failure") {
			if (conn_status == SessionWaitingAuthOK)
				this->notifyError(errorAuth);
			else
				this->notifyError(errorUnknown);
		}
		else if (treelist[i].getTag() == "message") {
			// Receives a message!
			if (treelist[i].hasAttributeValue("type","chat") and treelist[i].hasAttribute("from")) {
				unsigned long long time = 0;
				if (treelist[i].hasAttribute("t"))
					time = str2lng(treelist[i].getAttribute("t"));
				std::string from = treelist[i].getAttribute("from");
				std::string id = treelist[i].getAttribute("id");
				std::string author = treelist[i].getAttribute("author");
					
				Tree t = treelist[i].getChild("body");
				if (t.getTag() != "treeerr") {
					this->receiveMessage(ChatMessage(this,from,time,id,t.getData(),author));
				}
				t = treelist[i].getChild("media");
				if (t.getTag() != "treeerr") {
					if (t.hasAttributeValue("type","image")) {
						this->receiveMessage(
							ImageMessage(this, from, time, id, author, t.getAttribute("url"),
								str2int(t.getAttribute("width")), str2int(t.getAttribute("height")),
								str2int(t.getAttribute("size")), t.getAttribute("encoding"),
								t.getAttribute("filehash"),t.getAttribute("mimetype"),t.getData())
							);
					}
					else if (t.hasAttributeValue("type","location")) {
						this->receiveMessage(
							LocationMessage(this, from, time, id, author,
								str2dbl(t.getAttribute("latitude")), str2dbl(t.getAttribute("longitude")),
								t.getData())
							);
					}
					else if (t.hasAttributeValue("type","audio")) {
						this->receiveMessage(SoundMessage(this, from, time, id, author,
							t.getAttribute("url"),t.getAttribute("filehash"),t.getAttribute("mimetype"))
							);
					}
				}
				t = treelist[i].getChild("composing");
				if (t.getTag() != "treeerr") {
					this->gotTyping(from,"composing");
				}
				t = treelist[i].getChild("paused");
				if (t.getTag() != "treeerr") {
					this->gotTyping(from,"paused");
				}
			}
			else if (treelist[i].hasAttributeValue("type","notification") and treelist[i].hasAttribute("from")) {
				// If the nofitication comes from a group, assume we have to reload groups ;)
				updateGroups();
			}
			// Generate response for the messages
			if (treelist[i].hasAttribute("type") and treelist[i].hasAttribute("from")) { // and treelist[i].hasChild("notify")
				
				DataBuffer reply = generateResponse(treelist[i].getAttribute("from"),
													treelist[i].getAttribute("type"),
													treelist[i].getAttribute("id"));
				outbuffer = outbuffer + reply;
			}
		}
		else if (treelist[i].getTag() == "presence") {
			// Receives the presence of the user
			if ( treelist[i].hasAttribute("from") and treelist[i].hasAttribute("type") ) {
				this->notifyPresence(treelist[i].getAttribute("from"),treelist[i].getAttribute("type"));
			}
		}
		else if (treelist[i].getTag() == "iq") {
			// Receives the presence of the user
			if (atoi(treelist[i].getAttribute("id").c_str()) == gw1) gq_stat |= 2;
			if (atoi(treelist[i].getAttribute("id").c_str()) == gw2) gq_stat |= 4;
			
			if (treelist[i].hasAttributeValue("type","result") and treelist[i].hasAttribute("from")) {
				Tree t = treelist[i].getChild("query");
				if (t.getTag() != "treeerr") {
					if (t.hasAttributeValue("xmlns","jabber:iq:last") and
					    t.hasAttribute("seconds")) {

						this->notifyLastSeen(treelist[i].getAttribute("from"),t.getAttribute("seconds"));
					}
				}
				t = treelist[i].getChild("picture");
				if (t.getTag() != "treeerr") {
					if (t.hasAttributeValue("type","preview"))
						this->addPreviewPicture(treelist[i].getAttribute("from"),t.getData());
					if (t.hasAttributeValue("type","image"))
						this->addFullsizePicture(treelist[i].getAttribute("from"),t.getData());
				}
				std::vector <Tree> childs = treelist[i].getChildren();
				int acc = 0;
				for (int j = 0; j < childs.size(); j++) {
					if (childs[j].getTag() == "group") {
						bool rep = groups.find(getusername(childs[j].getAttribute("id"))) != groups.end();
						if (not rep) {
							groups.insert(  std::pair<std::string,Group> (
								getusername(childs[j].getAttribute("id")),
								Group(	getusername(childs[j].getAttribute("id")),
										childs[j].getAttribute("subject"),
										getusername(childs[j].getAttribute("owner")) )  )  );

							// Query group participants
							Tree iq("list", makeAttr1("xmlns","w:g"));
							Tree req("iq", makeAttr3("id",int2str(iqid++), "type","get", 
										"to",childs[j].getAttribute("id")+"@g.us"));
							req.addChild(iq);
							gw3++;
							outbuffer = outbuffer + serialize_tree(&req,true);
						}
					}
					else if (childs[j].getTag() == "participant") {
						std::string gid = getusername(treelist[i].getAttribute("from"));
						std::string pt = getusername(childs[j].getAttribute("jid"));
						if (groups.find(gid) != groups.end()) {
							groups.find(gid)->second.participants.push_back(pt);
						}
						if (!acc) gw3--;
						acc = 1;
					}
					else if (childs[j].getTag() == "add") {
						
					}
				}
				
				t = treelist[i].getChild("group");
				if (t.getTag() != "treeerr") {
					if (t.hasAttributeValue("type","preview"))
						this->addPreviewPicture(treelist[i].getAttribute("from"),t.getData());
					if (t.hasAttributeValue("type","image"))
						this->addFullsizePicture(treelist[i].getAttribute("from"),t.getData());
				}
			}
			if (treelist[i].hasAttribute("from") and treelist[i].hasAttribute("id") and treelist[i].hasChild("ping")) {
				this->doPong(treelist[i].getAttribute("id"),treelist[i].getAttribute("from"));
			}
		}
	}
	
	if (gq_stat == 8 and recv_messages_delay.size() != 0) {
		for (unsigned int i = 0; i < recv_messages_delay.size(); i++) {
			recv_messages.push_back(recv_messages_delay[i]);
		}
		recv_messages_delay.clear();
	}
}

DataBuffer WhatsappConnection::serialize_tree(Tree * tree, bool crypt) {
	DataBuffer data = write_tree(tree);
	unsigned char flag = 0;
	if (crypt) {
		data = data.encodedBuffer(this->out,this->session_key,true);
		flag = 0x10;
	}
	
	DataBuffer ret;
	ret.putInt(flag,1);
	ret.putInt(data.size(),2);
	ret = ret + data;
	return ret;
}

DataBuffer WhatsappConnection::write_tree(Tree * tree) {
	DataBuffer bout;
	int len = 1;
	
	if (tree->getAttributes().size() != 0) len += tree->getAttributes().size()*2;
	if (tree->getChildren().size() != 0) len++;
	if (tree->getData().size() != 0 or tree->forcedData()) len++;
	
	bout.writeListSize(len);
	if (tree->getTag() == "start") bout.putInt(1,1);
	else bout.putString(tree->getTag());
	tree->writeAttributes(&bout);
	
	if (tree->getData().size() > 0 or tree->forcedData())
		bout.putRawString(tree->getData());
	if (tree->getChildren().size() > 0) {
		bout.writeListSize(tree->getChildren().size());
		
		for (int i = 0; i < tree->getChildren().size(); i++) {
			DataBuffer tt = write_tree(&tree->getChildren()[i]);
			bout = bout + tt;
		}
	}
	return bout;
}


Tree WhatsappConnection::parse_tree(DataBuffer * data) {
	int bflag = (data->getInt(1) & 0xF0)>>4;
	int bsize = data->getInt(2,1);
	if (bsize > data->size()-3) {
		return Tree("treeerr");  // Next message incomplete, return consumed data
	}
	data->popData(3);	

	if (bflag & 8) {
		// Decode data, buffer conversion
		if (this->in != NULL) {
			DataBuffer * decoded_data = data->decodedBuffer(this->in,bsize,false);
			
			// Remove hash
			decoded_data->popData(4);
			
			// Call recursive
			data->popData(bsize); // Pop data unencrypted for next parsing!
			return read_tree(decoded_data);
		}else{
			printf("Received crypted data before establishing crypted layer! Skipping!\n");
			data->popData(bsize);
			return Tree("treeerr");
		}
	}else{
		return read_tree(data);
	}
}

Tree WhatsappConnection::read_tree(DataBuffer * data) {
	int lsize = data->readListSize();
	int type = data->getInt(1);
	if (type == 1) {
		data->popData(1);
		Tree t;
		t.readAttributes(data,lsize);
		t.setTag("start");
		return t;
	}else if (type == 2) {
		data->popData(1);
		return Tree("treeerr"); // No data in this tree...
	}
	
	Tree t;
	t.setTag(data->readString());
	t.readAttributes(data,lsize);
	
	if ((lsize & 1) == 1) {
		return t;
	}
	
	if (data->isList()) {
		t.setChildren(data->readList(this));
	}else{
		t.setData(data->readString());
	}
	
	return t;
}

static int isgroup(const std::string user) {
  return (user.find('-') != std::string::npos);
}

void WhatsappConnection::receiveMessage(const Message & m) {
	// Push message to user and generate a response
	Message * mc = m.copy();
	if (isgroup(m.from) and gq_stat != 8)  // Delay the group message deliver if we do not have the group list
		recv_messages_delay.push_back(mc);
	else
		recv_messages.push_back(mc);
	
	//std::cout << "Received message type " << m.type() << " from " << m.from << " at " << m.t << std::endl;
		
	// Now add the contact in the list (to query the profile picture)
	if (contacts.find(m.from) == contacts.end())
		contacts[m.from] = Contact(m.from,false);
	this->addContacts (std::vector <std::string> () );
}

void WhatsappConnection::notifyLastSeen(std::string from, std::string seconds) {
	from = getusername(from);
	contacts[from].last_seen = str2lng(seconds);
}

void WhatsappConnection::notifyPresence(std::string from, std::string status) {
	from = getusername(from);
	contacts[from].presence = status;
	user_changes.push_back(from);
}

void WhatsappConnection::addPreviewPicture(std::string from, std::string picture) {
	from = getusername(from);
	if (contacts.find(from) == contacts.end()) {
		Contact newc(from,false);
		contacts[from] = newc;
	}
	contacts[from].ppprev = picture;
	user_icons.push_back(from);
}

void WhatsappConnection::addFullsizePicture(std::string from, std::string picture) {
	if (contacts.find(from) == contacts.end()) {
		Contact newc(from,false);
		contacts[from] = newc;
	}
	contacts[from].pppicture = picture;
}

void WhatsappConnection::setMyPresence(std::string s, std::string msg) {
	if (s != mypresence) {
		mypresence = s;
		notifyMyPresence();
	}
	if (msg != mymessage) {
		mymessage = msg;
		notifyMyMessage(); //TODO
	}
}

void WhatsappConnection::notifyMyPresence() {
	// Send the nickname and the current status
	Tree pres("presence", makeAttr2("name",nickname, "type",mypresence));
	
	outbuffer = outbuffer + serialize_tree(&pres,true);
}

void WhatsappConnection::sendInitial() {
	Tree iq("iq", makeAttr3("id",int2str(iqid++), "type","get", "to",whatsappserver));
	Tree conf("config", makeAttr1("xmlns","urn:xmpp:whatsapp:push"));
	iq.addChild(conf);
	
	outbuffer = outbuffer + serialize_tree(&iq,true);
}

void WhatsappConnection::notifyMyMessage() {
	// Send the status message
	Tree xhash  ("x", makeAttr1("xmlns","jabber:x:event"));
	xhash.addChild(Tree("server"));
	Tree tbody("body"); tbody.setData(this->mymessage);
		
	Tree mes("message",makeAttr3("to","s.us","type","chat","id",int2str(time(NULL))+"-"+int2str(iqid++)));
	mes.addChild(xhash); mes.addChild(tbody);

	outbuffer = outbuffer + serialize_tree(&mes,true);
}

void WhatsappConnection::notifyError(ErrorCode err) {
	
}

bool WhatsappConnection::query_chat(std::string & from, std::string & message, std::string & author, unsigned long & t){
	for (int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 0) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			message = ((ChatMessage*)recv_messages[i])->message;
			author = ((ChatMessage*)recv_messages[i])->author;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin()+i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatimages(std::string & from, std::string & preview, std::string & url, unsigned long & t) {
	for (int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 1) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			preview = ((ImageMessage*)recv_messages[i])->preview;
			url = ((ImageMessage*)recv_messages[i])->url;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin()+i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatsounds(std::string & from, std::string & url, unsigned long & t) {
	for (int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 3) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			url = ((SoundMessage*)recv_messages[i])->url;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin()+i);
			return true;
		}
	}
	return false;
}

bool WhatsappConnection::query_chatlocations(std::string & from, double & lat, double & lng, std::string & prev, unsigned long & t) {
	for (int i = 0; i < recv_messages.size(); i++) {
		if (recv_messages[i]->type() == 2) {
			from = recv_messages[i]->from;
			t = recv_messages[i]->t;
			prev = ((LocationMessage*)recv_messages[i])->preview;
			lat = ((LocationMessage*)recv_messages[i])->latitude;
			lng = ((LocationMessage*)recv_messages[i])->longitude;
			delete recv_messages[i];
			recv_messages.erase(recv_messages.begin()+i);
			return true;
		}
	}
	return false;
}

int WhatsappConnection::getuserstatus(const std::string & who) {
	if (contacts.find(who) != contacts.end()) {
		if (contacts[who].presence == "available") return 1;
		return 0;
	}
	return -1;
}
std::string WhatsappConnection::getuserstatusstring(const std::string & who) {
	if (contacts.find(who) != contacts.end()) {
		return contacts[who].status;
	}
	return "";
}

unsigned long long WhatsappConnection::getlastseen(const std::string & who) {
	// Schedule a last seen update, just in case
	this->getLast(std::string(who)+"@"+whatsappserver);

	if (contacts.find(who) != contacts.end()) {
		return contacts[who].last_seen;
	}
	return ~0;
}

bool WhatsappConnection::query_status(std::string & from, int & status) {
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

bool WhatsappConnection::query_typing(std::string & from, int & status) {
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

bool WhatsappConnection::query_icon(std::string & from, std::string & icon, std::string & hash) {
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

void WhatsappConnection::doPong(std::string id, std::string from) {
	std::map <std::string,std::string> auth;
	auth["to"] = from;
	auth["id"] = id;
	auth["type"] = "result";
	Tree t("iq",auth);
	
	outbuffer = outbuffer + serialize_tree(&t,true);
}

void WhatsappConnection::sendResponse() {
	std::map <std::string,std::string> auth;
	auth["xmlns"] = "urn:ietf:params:xml:ns:xmpp-sasl";
	Tree t("response",auth);
	
	std::string response = phone + challenge_data + int2str(time(NULL));
	DataBuffer eresponse(response.c_str(),response.size());
	eresponse = eresponse.encodedBuffer(this->out,this->session_key,false);
	response = eresponse.toString();
	t.setData(response);

	outbuffer = outbuffer + serialize_tree(&t,false);
}

std::string WhatsappConnection::generateHeaders(std::string auth, int content_length) {
	std::string h = 
		"User-Agent: WhatsApp/2.4.7 S40Version/14.26 Device/Nokia302\r\n" 
		"Accept: text/json\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Authorization: " + auth + "\r\n"
		"Accept-Encoding: identity\r\n"
		"Content-Length: " + int2str(content_length) + "\r\n";
	return h;
}

std::string WhatsappConnection::generateHttpAuth(std::string nonce) {
	// cnonce is a 10 ascii char random string
	std::string cnonce;
	for (int i = 0; i < 10; i++)
		cnonce += ('a'+(rand()%25));

	std::string credentials = phone + ":s.whatsapp.net:" + base64_decode(password);
	std::string response = md5hex(  md5hex(md5raw(credentials)+":"+nonce+":"+cnonce)+
				":"+nonce+":00000001:"+cnonce+":auth:"+
				md5hex("AUTHENTICATE:WAWA/s.whatsapp.net") );
	
	return "X-WAWA: username=\"" + phone + "\",digest-uri=\"WAWA/s.whatsapp.net\"" + 
		",realm=\"s.whatsapp.net\",nonce=\"" + nonce + "\",cnonce=\"" + 
		cnonce + "\",nc=\"00000001\",qop=\"auth\",digest-uri=\"WAWA/s.whatsapp.net\"," + 
		"response=\"" + response + "\",charset=\"utf-8\"";
}

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
	bool query_chatsounds(std::string & from, std::string & url, unsigned long & t);
	bool query_chatlocations(std::string & from, double & lat, double & lng, std::string & prev, unsigned long & t);
	bool query_status(std::string & from, int & status);
	bool query_icon(std::string & from, std::string & icon, std::string & hash);
	bool query_typing(std::string & from, int & status);
	void account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status);
	void send_avatar(const std::string & avatar);
	int getuserstatus(const std::string & who);
	std::string getuserstatusstring(const std::string & who);
	unsigned long long getlastseen(const std::string & who);
	void addGroup(std::string subject);
	void leaveGroup(std::string group);
	void manageParticipant(std::string group, std::string participant, std::string command);
	
	void notifyTyping(std::string who, int status);
	void setMyPresence(std::string s, std::string msg);
	
	std::map <std::string,Group> getGroups();
	bool groupsUpdated();
		
	int loginStatus() const;

	int sendSSLCallback(char* buffer, int maxbytes);
	int sentSSLCallback(int bytessent);
	void receiveSSLCallback(char* buffer, int bytesrecv);
	bool hasSSLDataToSend();
	void SSLCloseCallback();
	bool hasSSLConnection(std::string & host, int * port);
};

WhatsappConnectionAPI::WhatsappConnectionAPI(std::string phone, std::string password, std::string nick) {
	connection = new WhatsappConnection(phone,password,nick);
}
WhatsappConnectionAPI::~WhatsappConnectionAPI() {
	delete connection;
}

std::map <std::string,Group> WhatsappConnectionAPI::getGroups() {
	return connection->getGroups();
}
bool WhatsappConnectionAPI::groupsUpdated() {
	return connection->groupsUpdated();
}

int WhatsappConnectionAPI::getuserstatus(const std::string & who) {
	return connection->getuserstatus(who);
}

void WhatsappConnectionAPI::addGroup(std::string subject) {
	connection->addGroup(subject);
}
void WhatsappConnectionAPI::leaveGroup(std::string subject) {
	connection->leaveGroup(subject);
}
void WhatsappConnectionAPI::manageParticipant(std::string group, std::string participant, std::string command) {
	connection->manageParticipant(group, participant, command);
}

unsigned long long WhatsappConnectionAPI::getlastseen(const std::string & who) {
	return connection->getlastseen(who);
}

void WhatsappConnectionAPI::send_avatar(const std::string & avatar) {
	connection->send_avatar(avatar);
}

bool WhatsappConnectionAPI::query_icon(std::string & from, std::string & icon, std::string & hash) {
	return connection->query_icon(from, icon, hash);
}

bool WhatsappConnectionAPI::query_typing(std::string & from, int & status) {
	return connection->query_typing(from,status);
}

void WhatsappConnectionAPI::setMyPresence(std::string s, std::string msg) {
	connection->setMyPresence(s,msg);
}

void WhatsappConnectionAPI::notifyTyping(std::string who, int status) {
	connection->notifyTyping(who,status);
}
std::string WhatsappConnectionAPI::getuserstatusstring(const std::string & who) {
	return connection->getuserstatusstring(who);
}

bool WhatsappConnectionAPI::query_chatimages(std::string & from, std::string & preview, std::string & url, unsigned long & t) {
	return connection->query_chatimages(from,preview,url,t);
}

bool WhatsappConnectionAPI::query_chatsounds(std::string & from, std::string & url, unsigned long & t) {
	return connection->query_chatsounds(from,url,t);
}

bool WhatsappConnectionAPI::query_chat(std::string & from, std::string & msg, std::string & author, unsigned long & t) {
	return connection->query_chat(from,msg,author,t);
}

bool WhatsappConnectionAPI::query_chatlocations(std::string & from, double & lat, double & lng, std::string & prev, unsigned long & t) {
	return connection->query_chatlocations(from,lat,lng,prev,t);
}

bool WhatsappConnectionAPI::query_status(std::string & from, int & status) {
	return connection->query_status(from,status);
}

void WhatsappConnectionAPI::sendChat(std::string to, std::string message) {
	connection->sendChat(to,message);
}
void WhatsappConnectionAPI::sendGroupChat(std::string to, std::string message) {
	connection->sendGroupChat(to,message);
}

int WhatsappConnectionAPI::loginStatus() const {
	return connection->loginStatus();
}

void WhatsappConnectionAPI::doLogin(std::string useragent) {
	connection->doLogin(useragent);
}
void WhatsappConnectionAPI::receiveCallback(const char * data, int len) {
	connection->receiveCallback(data,len);
}
int WhatsappConnectionAPI::sendCallback(char * data, int len) {
	return connection->sendCallback(data,len);
}
void WhatsappConnectionAPI::sentCallback(int len) {
	connection->sentCallback(len);
}
void WhatsappConnectionAPI::addContacts(std::vector <std::string> clist) {
	connection->addContacts(clist);
}
bool WhatsappConnectionAPI::hasDataToSend() {
	return connection->hasDataToSend();
}
void WhatsappConnectionAPI::account_info(unsigned long long & creation, unsigned long long & freeexp, std::string & status) {
	connection->account_info(creation,freeexp,status);
}


int WhatsappConnectionAPI::sendSSLCallback(char* buffer, int maxbytes) {
	return connection->sendSSLCallback(buffer,maxbytes);
}
int WhatsappConnectionAPI::sentSSLCallback(int bytessent) {
	return connection->sentSSLCallback(bytessent);
}
void WhatsappConnectionAPI::receiveSSLCallback(char* buffer, int bytesrecv) {
	connection->receiveSSLCallback(buffer,bytesrecv);
}
bool WhatsappConnectionAPI::hasSSLDataToSend() {
	return connection->hasSSLDataToSend();
}
void WhatsappConnectionAPI::SSLCloseCallback() {
	connection->SSLCloseCallback();
}
bool WhatsappConnectionAPI::hasSSLConnection(std::string & host, int * port) {
	return connection->hasSSLConnection(host,port);
}


static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}
std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}

