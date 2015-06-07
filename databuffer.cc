
#include <string.h>
#include <assert.h>

#include "databuffer.h"
#include "keygen.h"
#include "wadict.h"
#include "tree.h"
#include "wa_connection.h"

extern "C" {
	size_t tinfl_decompress_mem_to_mem(void *pOut_buf, size_t out_buf_len, const void *pSrc_buf, size_t src_buf_len, int flags);
}

// This changed to a huffman-like encoding. Therefore we can use one or two bytes...
static bool getDecoded(int n, std::string & res)
{
	if (n < 236) {
		res = std::string(main_dict[n]);
		return true;
	}
	return false; // Extended code!
}

static std::string getDecodedExtended(int n, int n2)
{
	if (n == 236) {
		return std::string(sec_dict[n2]);
	} // In the future add more secondary dictionaries here
	return ""; // This should never happen
}

// Return a token (or extended token) for a given value
static unsigned short lookupDecoded(std::string value)
{
	for (unsigned int i = 0; i < sizeof(main_dict)/sizeof(main_dict[0]); i++) {
		if (strcmp(main_dict[i], value.c_str()) == 0)
			return i;
	}
	// Not found in the main dict, search the secondary
	for (unsigned int i = 0; i < sizeof(sec_dict)/sizeof(sec_dict[0]); i++) {
		if (strcmp(sec_dict[i], value.c_str()) == 0)
			return i | 0x0100;
	}
	
	return 0;
}

DataBuffer::DataBuffer(const void *ptr, int size)
{
	if (ptr != NULL and size > 0) {
		buffer = (unsigned char *)malloc(size + 1);
		memcpy(buffer, ptr, size);
		blen = size;
	} else {
		blen = 0;
		buffer = (unsigned char *)malloc(1024);
	}
}

DataBuffer::~DataBuffer()
{
	free(buffer);
}

DataBuffer::DataBuffer(const DataBuffer & other)
{
	blen = other.blen;
	buffer = (unsigned char *)malloc(blen + 1024);
	memcpy(buffer, other.buffer, blen);
}

DataBuffer DataBuffer::operator+(const DataBuffer & other) const
{
	DataBuffer result = *this;

	result.addData(other.buffer, other.blen);
	return result;
}

DataBuffer & DataBuffer::operator =(const DataBuffer & other)
{
	if (this != &other) {
		free(buffer);
		this->blen = other.blen;
		buffer = (unsigned char *)malloc(blen + 1024);
		memcpy(buffer, other.buffer, blen);
	}
	return *this;
}

DataBuffer::DataBuffer(const DataBuffer * d)
{
	blen = d->blen;
	buffer = (unsigned char *)malloc(blen + 1024);
	memcpy(buffer, d->buffer, blen);
}

DataBuffer *DataBuffer::decodedBuffer(RC4Decoder * decoder, int clength, bool dout)
{
	DataBuffer *deco = new DataBuffer(this->buffer, clength);
	decoder->cipher(&deco->buffer[0], clength - 4);
	return deco;
}

DataBuffer *DataBuffer::decompressedBuffer()
{
	int ressize = this->size()*2;
	char dbuf[ressize];
	size_t outsize = tinfl_decompress_mem_to_mem(dbuf, ressize, this->buffer, this->size(), 1);

	if (outsize >= 0)
		return new DataBuffer(dbuf, outsize);
	
	return NULL;
}

DataBuffer DataBuffer::encodedBuffer(RC4Decoder * decoder, unsigned char *key, bool dout, unsigned int seq)
{
	DataBuffer deco = *this;
	decoder->cipher(&deco.buffer[0], blen);
	unsigned char hmacint[4];
	DataBuffer hmac;
	KeyGenerator::calc_hmac(deco.buffer, blen, key, (unsigned char *)&hmacint, seq);
	hmac.addData(hmacint, 4);

	if (dout)
		deco = deco + hmac;
	else
		deco = hmac + deco;

	return deco;
}

void DataBuffer::clear()
{
	blen = 0;
	free(buffer);
	buffer = (unsigned char *)malloc(1);
}

void *DataBuffer::getPtr()
{
	return buffer;
}

void DataBuffer::addData(const void *ptr, int size)
{
	if (ptr != NULL and size > 0) {
		buffer = (unsigned char *)realloc(buffer, blen + size);
		memcpy(&buffer[blen], ptr, size);
		blen += size;
	}
}

void DataBuffer::popData(int size)
{
	if (size > blen) {
		throw 0;
	} else {
		memmove(&buffer[0], &buffer[size], blen - size);
		blen -= size;
		if (blen + size > blen * 2 and blen > 8 * 1024)
			buffer = (unsigned char *)realloc(buffer, blen + 1);
	}
}

void DataBuffer::crunchData(int size)
{
	if (size > blen) {
		throw 0;
	} else {
		blen -= size;
	}
}

int DataBuffer::getInt(int nbytes, int offset)
{
	if (nbytes > blen)
		throw 0;
	int ret = 0;
	for (int i = 0; i < nbytes; i++) {
		ret <<= 8;
		ret |= buffer[i + offset];
	}
	return ret;
}

void DataBuffer::putInt(int value, int nbytes)
{
	assert(nbytes > 0);

	unsigned char out[nbytes];
	for (int i = 0; i < nbytes; i++) {
		out[nbytes - i - 1] = (value >> (i << 3)) & 0xFF;
	}
	this->addData(out, nbytes);
}

int DataBuffer::readInt(int nbytes)
{
	if (nbytes > blen)
		throw 0;
	int ret = getInt(nbytes);
	popData(nbytes);
	return ret;
}

int DataBuffer::readListSize()
{
	if (blen == 0)
		throw 0;
	int ret;
	if (buffer[0] == 0xf8 or buffer[0] == 0xf3) {
		ret = buffer[1];
		popData(2);
	} else if (buffer[0] == 0xf9) {
		ret = getInt(2, 1);
		popData(3);
	} else {
		/* FIXME throw 0 error */
		ret = -1;
		printf("Parse error!!\n");
	}
	return ret;
}

void DataBuffer::writeListSize(int size)
{
	if (size == 0) {
		putInt(0, 1);
	} else if (size < 256) {
		putInt(0xf8, 1);
		putInt(size, 1);
	} else {
		putInt(0xf9, 1);
		putInt(size, 2);
	}
}

std::string DataBuffer::readRawString(int size)
{
	if (size < 0 or size > blen)
		throw 0;
	std::string st(size, ' ');
	memcpy(&st[0], buffer, size);
	popData(size);
	return st;
}

std::string DataBuffer::readString()
{
	if (blen == 0)
		throw 0;
	int type = readInt(1);
	if (type > 2 and type <= 236) {
		std::string r;
		if (getDecoded(type,r))
			return r;
		return getDecodedExtended(type, readInt(1));
	} else if (type == 0) {
		return "";
	} else if (type == 252) {
		int slen = readInt(1);
		return readRawString(slen);
	} else if (type == 253) {
		int slen = readInt(3);
		return readRawString(slen);
	} else if (type == 250) {
		std::string u = readString();
		std::string s = readString();

		if (u.size() > 0 and s.size() > 0)
			return u + "@" + s;
		else if (s.size() > 0)
			return s;
		return "";
	}
	else if (type == 255) {
		// Some sort of number encoding (using 4 bit)
		int nbyte = readInt(1);
		int size = nbyte & 0x7f;
		int numnibbles = size*2 - ((nbyte&0x80) ? 1 : 0);

		std::string rawd = readRawString(size);
		std::string s;
		for (int i = 0; i < numnibbles; i++) {
			char c = (rawd[i/2] >> (4-((i&1)<<2))) & 0xF;
			if (c < 10) s += (c+'0');
			else s += (c-10+'-');
		}

		return s;
	}
	return "";
}

void DataBuffer::putRawString(std::string s)
{
	if (s.size() < 256) {
		putInt(0xfc, 1);
		putInt(s.size(), 1);
		addData(s.c_str(), s.size());
	} else {
		putInt(0xfd, 1);
		putInt(s.size(), 3);
		addData(s.c_str(), s.size());
	}
}

bool DataBuffer::canbeNibbled(const std::string & s) {
	for (unsigned i = 0; i < s.size(); i++) {
		if (!(
			(s[i] >= '0' && s[i] <= '9') ||
			(s[i] == '-') ||
			(s[i] == '.')
		))
			return false;
	}
	return true;
}

void DataBuffer::putString(std::string s)
{
	unsigned short lu = lookupDecoded(s);
	int sub_dict = (lu >> 8);
	
	if (sub_dict != 0)
		putInt(sub_dict + 236 - 1, 1);   // Put dict byte first!
	
	if (lu != 0) {
		putInt(lu & 0xFF, 1); // Now put second byte
	} else if (s.find('@') != std::string::npos) {
		std::string p1 = s.substr(0, s.find('@'));
		std::string p2 = s.substr(s.find('@') + 1);
		putInt(250, 1);
		putString(p1);
		putString(p2);
	} else if (canbeNibbled(s)) {
		// Encode it in nibbles
		int numn = (s.size()+1)/2;
		std::string out(numn, 0);
		for (unsigned i = 0; i < s.size(); i++) {
			unsigned char c;
			if (s[i] >= '0' && s[i] <= '9') c = s[i]-'0';
			else c = s[i]-'-'+10;

			out[i/2] |= c << (4-4*(i&1));
		}

		if (s.size() % 2 != 0) {
			numn |= 0x80;
			out[out.size()-1] |= 0xf;
		}
		putInt(255,1);
		putInt(numn,1);
		addData(out.c_str(), out.size());
	} else if (s.size() < 256) {
		putInt(252, 1);
		putInt(s.size(), 1);
		addData(s.c_str(), s.size());
	} else {
		putInt(253, 1);
		putInt(s.size(), 3);
		addData(s.c_str(), s.size());
	}
}

bool DataBuffer::isList()
{
	if (blen == 0)
		throw 0;
	return (buffer[0] == 248 or buffer[0] == 0 or buffer[0] == 249);
}

std::vector < Tree > DataBuffer::readList(WhatsappConnection * c)
{
	std::vector < Tree > l;
	int size = readListSize();
	while (size--) {
		Tree t;
		if (c->read_tree(this, t))
			l.push_back(t);
	}
	return l;
}

std::string DataBuffer::toString()
{
	std::string r(blen, ' ');
	memcpy(&r[0], buffer, blen);
	return r;
}

