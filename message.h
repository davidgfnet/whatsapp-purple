
#ifndef __MESSAGE__H__
#define __MESSAGE__H__

#include <string>
#include <vector>

#define CHAT_MESSAGE  0
#define IMAGE_MESSAGE 1
#define LOCAT_MESSAGE 2
#define SOUND_MESSAGE 3
#define VIDEO_MESSAGE 4

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

	virtual int type() const = 0;

	virtual Message *copy() const = 0;
};


class ChatMessage: public Message {
public:
	ChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string message, const std::string author);

	int type() const { return CHAT_MESSAGE; }
	std::string message;	/* Message */

	DataBuffer serialize() const;
	Message *copy() const;
};

class MediaMessage: public Message {
public:
	MediaMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author,
		const std::string url, const std::string hash, const std::string filetype);

	std::string url, hash, filetype;
};

class ImageMessage: public MediaMessage {
public:
	ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const unsigned int width,
		const unsigned int height, const unsigned int size, const std::string encoding, const std::string hash,
		const std::string filetype, const std::string preview);

	int type() const { return IMAGE_MESSAGE; }
	DataBuffer serialize() const;
	Message *copy() const;

	std::string encoding;
	std::string preview;
	unsigned int width, height, size;
};

class SoundMessage: public MediaMessage {
public:
	SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string hash,
		const std::string filetype);

	int type() const { return SOUND_MESSAGE; }
	Message *copy() const;
};

class VideoMessage:public MediaMessage {
public:
	VideoMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string hash,
		const std::string filetype);

	int type() const { return VIDEO_MESSAGE; }
	Message *copy() const;
};

class LocationMessage: public Message {
public:
	LocationMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, double lat, double lng, std::string preview);

	int type() const { return LOCAT_MESSAGE; }
	Message *copy() const;

	double latitude, longitude;	/* Location */
	std::string preview;
};

#endif

