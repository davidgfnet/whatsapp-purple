
#ifndef __DATABUFFER__H__
#define __DATABUFFER__H__

#include <string>
#include <vector>
#include "rc4.h"

class WhatsappConnection;
class Tree;

class DataBuffer {
private:
	unsigned char *buffer;
	int blen;
public:
	DataBuffer(const void *ptr = 0, int size = 0);
	~DataBuffer();
	DataBuffer(const DataBuffer & other);
	DataBuffer operator+(const DataBuffer & other) const;
	DataBuffer & operator =(const DataBuffer & other);
	DataBuffer(const DataBuffer * d);

	DataBuffer *decodedBuffer(RC4Decoder * decoder, int clength, bool dout);
	DataBuffer encodedBuffer(RC4Decoder * decoder, unsigned char *key, bool dout, unsigned int seq);
	DataBuffer *decompressedBuffer();

	void *getPtr();
	int getInt(int nbytes, int offset = 0);
	int size() { return blen; }

	int readInt(int nbytes);
	int readListSize();
	std::string readRawString(int size);
	std::string readString();
	std::vector < Tree > readList(WhatsappConnection * c);

	void putInt(int value, int nbytes);
	void writeListSize(int size);
	void putRawString(std::string s);
	void putString(std::string s);

	void clear();
	void addData(const void *ptr, int size);
	void popData(int size);
	void crunchData(int size);

	bool isList();
	std::string toString();
};

#endif

