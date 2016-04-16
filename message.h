
#ifndef __MESSAGE__H__
#define __MESSAGE__H__

#include <string>
#include <vector>

#include "AxolotlMessages.pb.h"

#define CHAT_MESSAGE  0
#define IMAGE_MESSAGE 1
#define LOCAT_MESSAGE 2
#define SOUND_MESSAGE 3
#define VIDEO_MESSAGE 4
#define CALL_MESSAGE  5
#define VCARD_MESSAGE 6

class WhatsappConnection;
class DataBuffer;
class Tree;

class Message {
public:
	Message(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id, const std::string author);
	virtual ~ Message() {}

	std::string from, server, author;
	unsigned long long t;
	std::string id;
	WhatsappConnection *wc;
	int retries;
	bool axolotl;

	virtual DataBuffer serialize() const = 0;
	virtual int type() const = 0;
	virtual Message *copy() const = 0;
};


class ChatMessage: public Message {
public:
	ChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string message, const std::string author);

	int type() const { return CHAT_MESSAGE; }
	std::string message;	/* Message */
	std::string ctype;
	std::string msg_body;

	DataBuffer serialize() const;
	Message *copy() const;
	std::string getProtoBuf() const;

	static ChatMessage parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string & buf);
};

class CipheredChatMessage: public ChatMessage {
public:
	CipheredChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string message, const std::string author, const std::string ctype);
};

class VCardMessage: public Message {
public:
	VCardMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string name, const std::string author, const std::string vcard);

	int type() const { return VCARD_MESSAGE; }
	std::string name, vcard;

	DataBuffer serialize() const;
	Message *copy() const;
};


class CallMessage: public Message {
public:
	CallMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id);

	int type() const { return CALL_MESSAGE; }

	DataBuffer serialize() const;
	Message *copy() const;
};


class MediaMessage: public Message {
public:
	MediaMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string caption,
		const std::string ip, const std::string hash, const std::string filetype);

	std::string url, caption, hash, filetype, ip;
};

class ImageMessage: public MediaMessage {
public:
	ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string caption, const std::string ip,
		const unsigned int width, const unsigned int height, const unsigned int size, const std::string encoding,
		const std::string hash,	const std::string filetype, const std::string preview);

	int type() const { return IMAGE_MESSAGE; }
	DataBuffer serialize() const;
	Message *copy() const;

	std::string encoding;
	std::string preview;
	unsigned int width, height, size;
	std::string e2e_key, e2e_iv, e2e_aeskey;

	static ImageMessage parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string & buf);
};

class SoundMessage: public MediaMessage {
public:
	SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string caption,
		const std::string hash, const std::string filetype);

	int type() const { return SOUND_MESSAGE; }
	DataBuffer serialize() const;
	Message *copy() const;
	DataBuffer serialize() const;
};

class VideoMessage:public MediaMessage {
public:
	VideoMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string caption,
		const std::string hash, const std::string filetype);

	int type() const { return VIDEO_MESSAGE; }
	Message *copy() const;
	DataBuffer serialize() const;
};

class LocationMessage: public Message {
public:
	LocationMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, double lat, double lng, const std::string name,
		std::string preview);

	int type() const { return LOCAT_MESSAGE; }
	Message *copy() const;
	DataBuffer serialize() const;

	double latitude, longitude;	/* Location */
	std::string name, preview;

	static LocationMessage parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string & buf);
};

#endif

