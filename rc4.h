
#ifndef __RC4__H__
#define __RC4__H__

class RC4Decoder {
public:
	RC4Decoder(const unsigned char *key, int keylen, int drop);
	void cipher(unsigned char *data, int len);

private:
	unsigned char s[256];
	unsigned char i, j;
	inline void swap(unsigned char i, unsigned char j)
	{
		unsigned char t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
};

#endif

