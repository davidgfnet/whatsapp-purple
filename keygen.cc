
#include "keygen.h"
#include <string.h>

#include "wa_util.h"

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_decode(std::string const &encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret += char_array_3[j];
	}

	return ret;
}

const static char hexmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

void KeyGenerator::generateKeyImei(const char *imei, const char *salt, int saltlen, char *out)
{
	char imeir[strlen(imei)];
	for (unsigned int i = 0; i < strlen(imei); i++)
		imeir[i] = imei[strlen(imei) - i - 1];

	char hash[16];
	MD5((unsigned char *)imeir, strlen(imei), (unsigned char *)hash);

	/* Convert to hex */
	char hashhex[32];
	for (int i = 0; i < 16; i++) {
		hashhex[2 * i] = hexmap[(hash[i] >> 4) & 0xF];
		hashhex[2 * i + 1] = hexmap[hash[i] & 0xF];
	}

	PKCS5_PBKDF2_HMAC_SHA1(hashhex, 32, (unsigned char *)salt, saltlen, 16, 20, (unsigned char *)out);
}

void KeyGenerator::generateKeyV2(const std::string pw, const char *salt, int saltlen, char *out)
{
	std::string dec = base64_decode(pw);

	PKCS5_PBKDF2_HMAC_SHA1(dec.c_str(), 20, (unsigned char *)salt, saltlen, 16, 20, (unsigned char *)out);
}

void KeyGenerator::generateKeysV14(const std::string pw, const char *salt, int saltlen, char *out)
{
	std::string dec = base64_decode(pw);
	char salt_[saltlen+1]; memcpy(salt_,salt,saltlen);
	
	for (int i = 0; i < 4; i++) {
		salt_[saltlen] = (i+1);
		PKCS5_PBKDF2_HMAC_SHA1(dec.c_str(), 20, (unsigned char *)salt_, saltlen+1, 2, 20, (unsigned char *)&out[20*i]);
	}
}

void KeyGenerator::generateKeyMAC(std::string macaddr, const char *salt, int saltlen, char *out)
{
	macaddr = macaddr + macaddr;

	char hash[16];
	MD5((unsigned char *)macaddr.c_str(), 34, (unsigned char *)hash);

	/* Convert to hex */
	char hashhex[32];
	for (int i = 0; i < 16; i++) {
		hashhex[2 * i] = hexmap[(hash[i] >> 4) & 0xF];
		hashhex[2 * i + 1] = hexmap[hash[i] & 0xF];
	}

	PKCS5_PBKDF2_HMAC_SHA1(hashhex, 32, (unsigned char *)salt, saltlen, 16, 20, (unsigned char *)out);
}

void KeyGenerator::calc_hmac_v12(const unsigned char *data, int l, const unsigned char *key, unsigned char *hmac)
{
	unsigned char temp[20];
	HMAC_SHA1(data, l, key, 20, temp);
	memcpy(hmac, temp, 4);
}

void KeyGenerator::calc_hmac(const unsigned char *data, int l, const unsigned char *key, unsigned char * hmac, unsigned int seq){
	unsigned char temp[20];
	unsigned char data_temp[l+4];
	memcpy(data_temp, data, l);
	data_temp[l  ] = (seq >> 24);
	data_temp[l+1] = (seq >> 16);
	data_temp[l+2] = (seq >>  8);
	data_temp[l+3] = (seq      );

	HMAC_SHA1 (data_temp,l+4,key,20,temp);
	
	memcpy(hmac,temp,4);
}

void KeyGenerator::HMAC_SHA1(const unsigned char *text, int text_len, const unsigned char *key, int key_len, unsigned char *digest)
{
	unsigned char SHA1_Key[4096], AppendBuf2[4096], szReport[4096];
	unsigned char *AppendBuf1 = new unsigned char[text_len + 64];
	unsigned char m_ipad[64], m_opad[64];

	memset(SHA1_Key, 0, 64);
	memset(m_ipad, 0x36, sizeof(m_ipad));
	memset(m_opad, 0x5c, sizeof(m_opad));

	if (key_len > 64)
		SHA1(key, key_len, SHA1_Key);
	else
		memcpy(SHA1_Key, key, key_len);

	for (unsigned int i = 0; i < sizeof(m_ipad); i++)
		m_ipad[i] ^= SHA1_Key[i];

	memcpy(AppendBuf1, m_ipad, sizeof(m_ipad));
	memcpy(AppendBuf1 + sizeof(m_ipad), text, text_len);

	SHA1(AppendBuf1, sizeof(m_ipad) + text_len, szReport);

	for (unsigned int j = 0; j < sizeof(m_opad); j++)
		m_opad[j] ^= SHA1_Key[j];

	memcpy(AppendBuf2, m_opad, sizeof(m_opad));
	memcpy(AppendBuf2 + sizeof(m_opad), szReport, 20);

	SHA1(AppendBuf2, sizeof(m_opad) + 20, digest);

	delete[]AppendBuf1;
}



