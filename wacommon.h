
#ifndef __WACOMMON__H__
#define __WACOMMON__H__

#include <string>
#include <map>
#include <stdlib.h>
#include <stdio.h>

static unsigned long long str2lng(std::string s)
{
	unsigned long long r;
	#ifdef _WIN32
	sscanf(s.c_str(), "%I64u", &r);
	#else
	sscanf(s.c_str(), "%llu", &r);
	#endif
	return r;
}

static std::string int2str(unsigned int num)
{
	char temp[512];
	sprintf(temp, "%d", num);
	return std::string(temp);
}

static int str2int(std::string s)
{
	int d;
	sscanf(s.c_str(), "%d", &d);
	return d;
}

static double str2dbl(std::string s)
{
  double f = (s[0] == '-') ? -1 : 1;
  while (s.size() > 0 && !(s[0] >= '0' && s[0] <= '9'))
    s = s.substr(1);

  int i1 = atoi(s.c_str());
  do {
    s = s.substr(1);
  } while (s.size() > 0 && s[0] >= '0' && s[0] <= '9');
  s = s.substr(1);
  int i2 = atoi(s.c_str());

  double d = i2;
  while (d > 1.0f || d < -1.0f)
    d /= 10;
  d += i1;

  return d*f;
}

static std::string getusername(std::string user)
{
	size_t pos = user.find('@');
	if (pos != std::string::npos)
		return user.substr(0, pos);
	else
		return user;
}

#define makeAttr1(k1,v1) std::map<std::string, std::string> ({ {k1,v1} })
#define makeAttr2(k1,v1,k2,v2) std::map<std::string, std::string> ({ {k1,v1},{k2,v2} })
#define makeAttr3(k1,v1,k2,v2,k3,v3) std::map<std::string, std::string> ({ {k1,v1},{k2,v2},{k3,v3} })
#define makeAttr4(k1,v1,k2,v2,k3,v3,k4,v4) std::map<std::string, std::string> ({ {k1,v1},{k2,v2},{k3,v3},{k4,v4} })
#define makeAttr5(k1,v1,k2,v2,k3,v3,k4,v4,k5,v5) std::map<std::string, std::string> ({ {k1,v1},{k2,v2},{k3,v3},{k4,v4},{k5,v5} })


#endif

