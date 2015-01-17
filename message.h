
#ifndef __MESSAGE__H__
#define __MESSAGE__H__

#include <string>
#include <vector>

#include "databuffer.h"
#include "tree.h"

class WhatsappConnection;
class DataBuffer;

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

	int type() const { return 0; }
	std::string message;	/* Message */

	DataBuffer serialize() const;
	Message *copy() const;
};

class ImageMessage: public Message {
public:
	ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const unsigned int width,
		const unsigned int height, const unsigned int size, const std::string encoding, const std::string hash,
		const std::string filetype, const std::string preview);

	int type() const { return 1; }
	DataBuffer serialize() const;
	Message *copy() const;

	std::string url;	/* Image URL */
	std::string encoding, hash, filetype;
	std::string preview;
	unsigned int width, height, size;
};

class SoundMessage: public Message {
public:
	SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string hash,
		const std::string filetype);

	int type() const { return 3; }
	Message *copy() const;
	std::string url;	/* Sound URL */
	std::string hash, filetype;
};

class VideoMessage:public Message {
public:
	VideoMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string url, const std::string hash,
		const std::string filetype);

	int type() const { return 4; }
	Message *copy() const;

	std::string url;	/* Video URL */
	std::string hash, filetype;
};

class LocationMessage: public Message {
public:
	LocationMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, double lat, double lng, std::string preview);

	int type() const { return 2; }
	Message *copy() const;

	double latitude, longitude;	/* Location */
	std::string preview;
};

#endif

