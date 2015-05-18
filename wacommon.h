
#ifndef __WACOMMON__H__
#define __WACOMMON__H__

#include <string>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <locale>

static double str2dbl(std::string s)
{
	float longitude = 0.0f;
	std::istringstream istr(s);
	istr.imbue(std::locale("C"));
	istr >> longitude;
	return longitude;
}

static std::string getusername(std::string user)
{
	size_t pos = user.find('@');
	if (pos != std::string::npos)
		return user.substr(0, pos);
	else
		return user;
}

static std::map<std::string, std::string> makeat(std::vector <std::string> v) {
	std::map<std::string, std::string> ret;
	for (unsigned i = 0; i < v.size(); i+= 2)
		ret[v[i]] = v[i+1];
	return ret;
}

#endif

