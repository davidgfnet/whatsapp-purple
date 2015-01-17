
#include "message.h"
#include "wacommon.h"
#include "wa_connection.h"

Message::Message(const WhatsappConnection * wc, const std::string from, const unsigned long long time, const std::string id, const std::string author)
{
	size_t pos = from.find('@');
	if (pos != std::string::npos) {
		this->from = from.substr(0, pos);
		this->server = from.substr(pos + 1);
	} else
		this->from = from;
	this->t = time;
	this->wc = const_cast < WhatsappConnection * >(wc);
	this->id = id;
	this->author = getusername(author);
}


ChatMessage::ChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string message, const std::string author) : 
	Message(wc, from, time, id, author)
{
	this->message = message;
}

DataBuffer ChatMessage::serialize() const
{
	Tree tbody("body");
	tbody.setData(this->message);

	std::string stime = int2str(t);
	std::map < std::string, std::string > attrs;
	if (server.size())
		attrs["to"] = from + "@" + server;
	else
		attrs["to"] = from + "@" + wc->whatsappserver;
	attrs["type"] = "text";
	attrs["id"] = id;
	attrs["t"] = stime;

	Tree mes("message", attrs);
	mes.addChild(tbody);

	return wc->serialize_tree(&mes);
}

Message *ChatMessage::copy() const
{
	return new ChatMessage(wc, from, t, id, message, author);
}


ImageMessage::ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, 
	const std::string id, const std::string author, const std::string url, const unsigned int width,
	const unsigned int height, const unsigned int size, const std::string encoding, const std::string hash,
	const std::string filetype, const std::string preview)
	:Message(wc, from, time, id, author)
{

	this->url = url;
	this->width = width;
	this->height = height;
	this->size = size;
	this->encoding = encoding;
	this->hash = hash;
	this->filetype = filetype;
	this->preview = preview;
}

DataBuffer ImageMessage::serialize() const
{
	Tree tmedia("media", makeAttr4("type", "image", "url", url, "size", int2str(size), "file", "myfile.jpg"));
	tmedia.setData(preview);	/* ICON DATA! */

	std::string stime = int2str(t);
	std::map < std::string, std::string > attrs;
	if (server.size())
		attrs["to"] = from + "@" + server;
	else
		attrs["to"] = from + "@" + wc->whatsappserver;
	attrs["type"] = "media";
	attrs["id"] = id;
	attrs["t"] = stime;

	Tree mes("message", attrs);
	mes.addChild(tmedia);

	return wc->serialize_tree(&mes);
}

Message *ImageMessage::copy() const
{
	return new ImageMessage(wc, from, t, id, author, url, width, height, size, encoding, hash, filetype, preview);
}


SoundMessage::SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string url, const std::string hash,
	const std::string filetype)
	:Message(wc, from, time, id, author)
{
	this->url = url;
	this->filetype = filetype;
}

Message * SoundMessage::copy() const
{
	return new SoundMessage(wc, from, t, id, author, url, hash, filetype);
}


VideoMessage::VideoMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string url, const std::string hash,
	const std::string filetype)
	:Message(wc, from, time, id, author)
{
	this->url = url;
	this->filetype = filetype;
}

Message * VideoMessage::copy() const
{
	return new VideoMessage(wc, from, t, id, author, url, hash, filetype);
}


LocationMessage::LocationMessage(const WhatsappConnection * wc, const std::string from,
	const unsigned long long time, const std::string id, const std::string author,
	double lat, double lng, std::string preview)
	:Message(wc, from, time, id, author)
{
	this->latitude = lat;
	this->longitude = lng;
	this->preview = preview;
}

Message * LocationMessage::copy() const
{
	return new LocationMessage(wc, from, t, id, author, latitude, longitude, preview);
}



