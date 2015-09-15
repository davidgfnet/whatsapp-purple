
#include "serializer.h"

void Serializer::putInt(uint64_t v, int size) {
	for (unsigned int i = 0; i < size; i++) {
		char c = v & 0xFF;
		buffer += c;
		v = v >> 8;
	}
}

void Serializer::putString(std::string s) {
	putInt32(s.size());
	buffer += s;
}



uint64_t Unserializer::readInt(int size) {
	uint64_t ret = 0;
	for (uint64_t i = 0; i < size; i++) {
		ret |= (buffer[i] << (i*8));
	}
	buffer = buffer.substr(4);
	return ret;
}

std::string Unserializer::readString() {
	unsigned int length = readInt32();
	std::string ret(buffer.c_str(), length);

	buffer = buffer.substr(length);
	return ret;
}


