#ifndef __BYTEUTIL_H
#define __BYTEUTIL_H

#include <string>
#include <vector>
#include <stdint.h>

#define ByteArray std::string

class ByteUtil
{
public:
	ByteUtil();

	static ByteArray combine(const std::vector<ByteArray> &items);
	static std::vector<ByteArray> split(const ByteArray &input, int firstLength, int secondLength, int thirdLength = -1);
	static ByteArray trim(const ByteArray &input, int length);
	static uint8_t intsToByteHighAndLow(int highValue, int lowValue);
	static int highBitsToInt(uint8_t input);
	static int lowBitsToInt(uint8_t input);
	static int intToByteArray(ByteArray &input, int offset, int value);
	static ByteArray toHex(ByteArray in);
};

#endif // BYTEUTIL_H
