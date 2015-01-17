
#ifndef __KEYGENWA__H__
#define __KEYGENWA__H__

#include <string>

class KeyGenerator {
public:
	// Legacy key deriv methods
	static void generateKeyImei(const char *imei, const char *salt, int saltlen, char *out);
	static void generateKeyMAC(std::string macaddr, const char *salt, int saltlen, char *out);
	static void generateKeyV2(const std::string pw, const char *salt, int saltlen, char *out);

	// New way to derive key.
	static void generateKeysV14(const std::string pw, const char *salt, int saltlen, char *out);

	// HMAC generation for CRC checking
	static void calc_hmac_v12(const unsigned char *data, int l, const unsigned char *key, unsigned char *hmac);
	static void calc_hmac(const unsigned char *data, int l, const unsigned char *key, unsigned char * hmac, unsigned int seq);

private:
	static void HMAC_SHA1(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest);
};

#endif

