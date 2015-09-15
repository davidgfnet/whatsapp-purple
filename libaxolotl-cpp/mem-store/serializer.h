
#ifndef SERIALIZER__H_
#define SERIALIZER__H_

#include <string>

class Serializer {
public:
	void putInt32(unsigned int v) { putInt(v, 4); }
	void putInt64(uint64_t v) { putInt(v, 8); }

	void putInt(uint64_t v, int size);

	void putString(std::string s);

	std::string getBuffer() const { return buffer; }

private:
	std::string buffer;
};

class Unserializer {
public:
	Unserializer(std::string s) : buffer(s) {}

	unsigned int readInt32() { return readInt(4); }
	uint64_t readInt64() { return readInt(8); }
	uint64_t readInt(int size);

	std::string readString();

private:
	std::string buffer;
};


#endif

