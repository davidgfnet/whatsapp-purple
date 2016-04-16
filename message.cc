
#include "message.h"
#include "wacommon.h"
#include "wa_connection.h"
#include "tree.h"
#include "wa_util.h"

using namespace wapurple;

std::string basename(std::string s) {
	while (s.find("/") != std::string::npos)
		s = s.substr(s.find("/")+1);
	return s;
}

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
	this->retries = 0;
	this->axolotl = true;
}

MediaMessage::MediaMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string url, const std::string caption, 
	const std::string ip, const std::string hash, const std::string filetype)
 : Message(wc, from, time, id, author),
   url(url), caption(caption), hash(hash), filetype(filetype), ip(ip)
{
}

ChatMessage::ChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string message, const std::string author) : 
	Message(wc, from, time, id, author)
{
	this->message = message;
	this->msg_body = "body";
}

DataBuffer ChatMessage::serialize() const
{
	Tree tbody(this->msg_body);
	tbody.setData(this->message);
	if (this->ctype != "") {
		tbody.setAttributes(makeat({"type", this->ctype, "v", "2", "count", "1"}));
	}

	std::string stime = std::to_string(t);
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

ChatMessage ChatMessage::parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string & buf) {

	AxolotlMessage pbuf;
	pbuf.ParseFromString(buf);

	return ChatMessage(wc, from, time, id, pbuf.textmsg(), author);
}

std::string ChatMessage::getProtoBuf() const {
	AxolotlMessage pbuf;
	pbuf.set_textmsg(message);

	std::string ret;
	pbuf.SerializeToString(&ret);

	return ret + '\1';
}


CipheredChatMessage::CipheredChatMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string message, const std::string author, const std::string ctype) : 
	ChatMessage(wc, from, time, id, message, author)
{
	this->msg_body = "enc";
	this->ctype = ctype;
}

CallMessage::CallMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id) : 
	Message(wc, from, time, id, "")
{
}

DataBuffer CallMessage::serialize() const
{
	Tree mes("call");
	return wc->serialize_tree(&mes);
}

Message *CallMessage::copy() const
{
	return new CallMessage(wc, from, t, id);
}

ImageMessage::ImageMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time, 
	const std::string id, const std::string author, const std::string url, const std::string caption,
	const std::string ip, const unsigned int width, const unsigned int height, const unsigned int size,
	const std::string encoding, const std::string hash, const std::string filetype, const std::string preview)
	:MediaMessage(wc, from, time, id, author, url, caption, ip, hash, filetype)
{
	this->width = width;
	this->height = height;
	this->size = size;
	this->encoding = encoding;
	this->preview = preview;
}

DataBuffer ImageMessage::serialize() const
{
	std::map < std::string, std::string > mattrs;
	mattrs["encoding"] = "raw";
	mattrs["filehash"] = this->hash;
	mattrs["mimetype"] = this->filetype;
	mattrs["width"] = std::to_string(this->width);
	mattrs["height"] = std::to_string(this->height);
	mattrs["type"] = "image";
	mattrs["url"] = url;
	mattrs["size"] = std::to_string(size);
	mattrs["file"] = basename(url);
	mattrs["ip"] = this->ip;

	Tree tmedia("media", mattrs);
	tmedia.setData(preview);	/* ICON DATA! */

	std::string stime = std::to_string(t);
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
	ImageMessage * im = new ImageMessage(wc, from, t, id, author, url, caption, ip, width, height, size, encoding, hash, filetype, preview);
	im->e2e_key    = this->e2e_key;
	im->e2e_aeskey = this->e2e_aeskey;
	im->e2e_iv     = this->e2e_iv;

	return im;
}


ImageMessage ImageMessage::parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string & buf) {

	AxolotlMessage pbuf;
	pbuf.ParseFromString(buf);

	ImageMessage im(wc, from, time, id, author,
		pbuf.imagemsg().url(), pbuf.imagemsg().caption(), "",
		pbuf.imagemsg().width(), pbuf.imagemsg().height(), pbuf.imagemsg().length(), "",
		pbuf.imagemsg().sha256(), pbuf.imagemsg().mimetype(), pbuf.imagemsg().thumbnail());

	// Fill in the RefKey needed to decrypt the image
	im.e2e_key = pbuf.imagemsg().refkey();

	// Calculate keys
	HKDF keygen(3);
	std::string keys = keygen.deriveSecrets(im.e2e_key, "WhatsApp Image Keys", 112, "");
	im.e2e_iv = keys.substr(0, 16);
	im.e2e_aeskey = keys.substr(16, 32);
	//std::strings macKey = keys.substr(48, 80);
	//std::strings refKey = keys.substr(80);

	return im;
}

SoundMessage::SoundMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string url, const std::string caption,
	const std::string hash, const std::string filetype)
	:MediaMessage(wc, from, time, id, author, url, caption, "", hash, filetype)
{
}

DataBuffer SoundMessage::serialize() const
{
        std::map < std::string, std::string > mattrs;
        mattrs["encoding"] = "raw";
        mattrs["filehash"] = this->hash;
        mattrs["mimetype"] = this->filetype;
        mattrs["type"] = "audio";
        mattrs["url"] = url;
        mattrs["file"] = basename(url);
        mattrs["ip"] = this->ip;

        Tree tmedia("media", mattrs);

        std::string stime = std::to_string(t);
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

Message * SoundMessage::copy() const
{
	return new SoundMessage(wc, from, t, id, author, url, caption, hash, filetype);
}

VideoMessage::VideoMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string url, const std::string caption, const std::string hash,
	const std::string filetype)
	:MediaMessage(wc, from, time, id, author, url, caption, "", hash, filetype)
{
}

Message * VideoMessage::copy() const
{
	return new VideoMessage(wc, from, t, id, author, url, caption, hash, filetype);
}


LocationMessage::LocationMessage(const WhatsappConnection * wc, const std::string from,
	const unsigned long long time, const std::string id, const std::string author,
	double lat, double lng, const std::string name, std::string preview)
	:Message(wc, from, time, id, author), latitude(lat), longitude(lng),
	name(name), preview(preview)
{
}

Message * LocationMessage::copy() const
{
	return new LocationMessage(wc, from, t, id, author, latitude, longitude, name, preview);
}

LocationMessage LocationMessage::parseProtobuf(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
	const std::string id, const std::string author, const std::string & buf) {

	AxolotlMessage pbuf;
	pbuf.ParseFromString(buf);

	return LocationMessage(wc, from, time, id, author, pbuf.locationmsg().latitude(), 
		pbuf.locationmsg().longitude(), pbuf.locationmsg().name() + " (" + pbuf.locationmsg().address() + ")",
		pbuf.locationmsg().jpeg_thumbnail());
}

VCardMessage::VCardMessage(const WhatsappConnection * wc, const std::string from, const unsigned long long time,
		const std::string id, const std::string author, const std::string name, const std::string vcard) 
	: Message(wc, from, time, id, author), name(name), vcard(vcard) 
{}

Message *VCardMessage::copy() const
{
	return new VCardMessage(wc, from, t, id, author, name, vcard);
}

DataBuffer VCardMessage::serialize() const
{
	Tree vcardt("vcard", makeat({"name", this->name}));
	vcardt.setData(this->vcard);
	Tree tmedia("media", makeat({"encoding", "text", "type", "vcard"}));
	tmedia.addChild(vcardt);

	std::string stime = std::to_string(t);
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

DataBuffer LocationMessage::serialize() const
{
	return DataBuffer();
}

DataBuffer VideoMessage::serialize() const
{
	return DataBuffer();
}

DataBuffer SoundMessage::serialize() const
{
	return DataBuffer();
}



