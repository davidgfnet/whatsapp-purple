
#include "sqliutil.h"

#include <algorithm>    // std::copy
#include <vector>       // std::vector
#include <string>

std::vector <unsigned char> string_to_barray(std::string s) {
	std::vector <unsigned char> v;
	std::copy( s.begin(), s.end(), std::back_inserter(v));
	return v;
}

std::string barray_to_string(std::vector <unsigned char> v) {
	std::string ret;
	for (auto c: v)
		ret += (char)c;
	return ret;
}

