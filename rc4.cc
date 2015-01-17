
#include "rc4.h"

RC4Decoder::RC4Decoder(const unsigned char *key, int keylen, int drop)
{
	for (unsigned int k = 0; k < 256; k++)
		s[k] = k;
	i = j = 0;
	do {
		unsigned char k = key[i % keylen];
		j = (j + k + s[i]) & 0xFF;
		swap(i, j);
	} while (++i != 0);
	i = j = 0;

	unsigned char temp[drop];
	for (int k = 0; k < drop; k++)
		temp[k] = 0;
	cipher(temp, drop);
}

void RC4Decoder::cipher(unsigned char *data, int len)
{
	while (len--) {
		i++;
		j += s[i];
		swap(i, j);
		unsigned char idx = s[i] + s[j];
		*data++ ^= s[idx];
	}
}

