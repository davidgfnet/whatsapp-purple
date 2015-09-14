#include "byteutil.h"

ByteUtil::ByteUtil()
{
}

ByteArray ByteUtil::combine(const std::vector<ByteArray> &items)
{
	ByteArray result;
	for (const ByteArray &item: items) {
		result += item;
	}
	return result;
}

std::vector<ByteArray> ByteUtil::split(const ByteArray &input, int firstLength, int secondLength, int thirdLength)
{
	std::vector<ByteArray> result;
	result.push_back(input.substr(0, firstLength));
	result.push_back(input.substr(firstLength, secondLength));
	if (thirdLength > -1) {
		result.push_back(input.substr(firstLength + secondLength, thirdLength));
	}
	return result;
}

ByteArray ByteUtil::trim(const ByteArray &input, int length)
{
	return input.substr(0, length);
}

uint8_t ByteUtil::intsToByteHighAndLow(int highValue, int lowValue)
{
	return (highValue << 4 | lowValue) & 0xFF;
}

int ByteUtil::highBitsToInt(uint8_t input)
{
	return (input & 0xFF) >> 4;
}

int ByteUtil::lowBitsToInt(uint8_t input)
{
	return input & 0xF;
}

int ByteUtil::intToByteArray(ByteArray &input, int offset, int value)
{
	input[offset + 3] = (char)(value % 256);
	input[offset + 2] = (char)((value >> 8) % 256);
	input[offset + 1] = (char)((value >> 16) % 256);
	input[offset]	 = (char)((value >> 24) % 256);
	return 4;
}

ByteArray ByteUtil::toHex(ByteArray in)
{
	std::string map = "0123456789abcdef";
	std::string ret;
	for (auto c: in) {
		ret += map[(c>>4)&0xF];
		ret += map[(c)&0xF];
	}

	return ret;
}

ByteArray ByteUtil::fromHex(ByteArray in)
{
	std::string map = "0123456789abcdef";
	std::string ret;
	for (unsigned i = 0; i < in.size(); i += 2) {
		int n1 = map.find(in[i]);
		int n2 = map.find(in[i+1]);
		int res = (n1 << 4) | n2;
		ret += (char)res;
	}
	return ret;
}


