
#ifndef SQLIUTIL_H__
#define SQLIUTIL_H__

#include <string>
#include <vector>

std::vector <unsigned char> string_to_barray(std::string s);
std::string barray_to_string(std::vector <unsigned char> v);

#endif

