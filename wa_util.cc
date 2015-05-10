
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#include <string>
#include <stdio.h>

#include "wa_util.h"
#include "imgutil.h"

/* Implementations when Openssl is not present */

#ifndef ENABLE_OPENSSL

unsigned char *MD5(const unsigned char *d, int n, unsigned char *md)
{
	PurpleCipher *md5_cipher;
	PurpleCipherContext *md5_ctx;

	md5_cipher = purple_ciphers_find_cipher("md5");
	md5_ctx = purple_cipher_context_new(md5_cipher, NULL);
	purple_cipher_context_append(md5_ctx, (guchar *) d, n);
	purple_cipher_context_digest(md5_ctx, 16, md, NULL);
	purple_cipher_context_destroy(md5_ctx);

	return md;
}

unsigned char *SHA1(const unsigned char *d, int n, unsigned char *md)
{
	PurpleCipher *sha1_cipher;
	PurpleCipherContext *sha1_ctx;

	sha1_cipher = purple_ciphers_find_cipher("sha1");
	sha1_ctx = purple_cipher_context_new(sha1_cipher, NULL);
	purple_cipher_context_append(sha1_ctx, (guchar *) d, n);
	purple_cipher_context_digest(sha1_ctx, 20, md, NULL);
	purple_cipher_context_destroy(sha1_ctx);

	return md;
}

#endif

const char hmap[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

std::string tohex(const char *t, int l)
{
	std::string ret;
	for (int i = 0; i < l; i++) {
		ret += hmap[((*t) >> 4) & 0xF];
		ret += hmap[((*t++)) & 0xF];
	}
	return ret;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789+/";

std::string base64_encode_esp(unsigned char const *bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		for (j = i; j < 3; j++)
			ret += "=";

	}

	return ret;

}

std::string getpreview(const char *filename)
{
	FILE * fd = fopen(filename, "rb");
	std::string r;
	int read = 0;
	do {
		char buf[1024];
		read = fread(buf, 1, 1024, fd);
		if (read > 0)
			r += std::string(buf,read);
	} while (read > 0);
	fclose(fd);

	int len;
	char * buffer;
	imgThumbnail((unsigned char*)r.c_str(), r.size(), (void**)&buffer, &len, 81);

	std::string ret(buffer, len);
	return ret;
}

std::string SHA256_file_b64(const char *filename)
{
	unsigned char md[32];

	PurpleCipher *sha_cipher;
	PurpleCipherContext *sha_ctx;

	sha_cipher = purple_ciphers_find_cipher("sha256");
	sha_ctx = purple_cipher_context_new(sha_cipher, NULL);

	FILE *fd = fopen(filename, "rb");
	int read = 0;
	do {
		char buf[1024];
		read = fread(buf, 1, 1024, fd);
		purple_cipher_context_append(sha_ctx, (guchar *) buf, read);
	} while (read > 0);
	fclose(fd);

	purple_cipher_context_digest(sha_ctx, 32, md, NULL);
	purple_cipher_context_destroy(sha_ctx);

	return base64_encode_esp(md, 32);
}

std::string md5hex(std::string target)
{
	char outh[16];
	MD5((unsigned char *)target.c_str(), target.size(), (unsigned char *)outh);
	return tohex(outh, 16);
}

std::string md5raw(std::string target)
{
	char outh[16];
	MD5((unsigned char *)target.c_str(), target.size(), (unsigned char *)outh);
	return std::string(outh, 16);
}

#ifndef ENABLE_OPENSSL
int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter, int keylen, unsigned char *out)
{
	unsigned char digtmp[20], *p, itmp[4];
	int cplen, j, k, tkeylen;
	int mdlen = 20;		/* SHA1 */
	unsigned long i = 1;

	PurpleCipherContext *context = purple_cipher_context_new_by_name("hmac", NULL);

	p = out;
	tkeylen = keylen;
	while (tkeylen) {
		if (tkeylen > mdlen)
			cplen = mdlen;
		else
			cplen = tkeylen;
		/* We are unlikely to ever use more than 256 blocks (5120 bits!)
		 * but just in case...
		 */
		itmp[0] = (unsigned char)((i >> 24) & 0xff);
		itmp[1] = (unsigned char)((i >> 16) & 0xff);
		itmp[2] = (unsigned char)((i >> 8) & 0xff);
		itmp[3] = (unsigned char)(i & 0xff);

		purple_cipher_context_reset(context, NULL);
		purple_cipher_context_set_option(context, "hash", (gpointer) "sha1");
		purple_cipher_context_set_key_with_len(context, (guchar *) pass, passlen);
		purple_cipher_context_append(context, (guchar *) salt, saltlen);
		purple_cipher_context_append(context, (guchar *) itmp, 4);
		purple_cipher_context_digest(context, mdlen, digtmp, NULL);

		memcpy(p, digtmp, cplen);
		for (j = 1; j < iter; j++) {

			purple_cipher_context_reset(context, NULL);
			purple_cipher_context_set_option(context, "hash", (gpointer) "sha1");
			purple_cipher_context_set_key_with_len(context, (guchar *) pass, passlen);
			purple_cipher_context_append(context, (guchar *) digtmp, mdlen);
			purple_cipher_context_digest(context, mdlen, digtmp, NULL);

			for (k = 0; k < cplen; k++)
				p[k] ^= digtmp[k];
		}
		tkeylen -= cplen;
		i++;
		p += cplen;
	}

	purple_cipher_context_destroy(context);

	return 1;
}
#endif

/* MIME type, copied from mxit */
#define		MIME_TYPE_OCTETSTREAM	"application/octet-stream"
#define		ARRAY_SIZE( x )		( sizeof( x ) / sizeof( x[0] ) )

/* supported file mime types */
static struct mime_type {
	const char *magic;
	const short magic_len;
	const char *mime;
}

const mime_types[] = {
	/* images */
	{"\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8, "image/png"},		/* image png */
	{"\xFF\xD8", 2, "image/jpeg"},					/* image jpeg */
	{"\x3C\x3F\x78\x6D\x6C", 5, "image/svg+xml"},			/* image SVGansi */
	{"\xEF\xBB\xBF", 3, "image/svg+xml"},				/* image SVGutf */
	{"\xEF\xBB\xBF", 3, "image/svg+xml"},				/* image SVGZ */
	/* mxit */
	{"\x4d\x58\x4d", 3, "application/mxit-msgs"},			/* mxit message */
	{"\x4d\x58\x44\x01", 4, "application/mxit-mood"},		/* mxit mood */
	{"\x4d\x58\x45\x01", 4, "application/mxit-emo"},		/* mxit emoticon */
	{"\x4d\x58\x46\x01", 4, "application/mxit-emof"},		/* mxit emoticon frame */
	{"\x4d\x58\x53\x01", 4, "application/mxit-skin"},		/* mxit skin */
	/* audio */
	{"\x4d\x54\x68\x64", 4, "audio/midi"},				/* audio midi */
	{"\x52\x49\x46\x46", 4, "audio/wav"},				/* audio wav */
	{"\xFF\xF1", 2, "audio/aac"},					/* audio aac1 */
	{"\xFF\xF9", 2, "audio/aac"},					/* audio aac2 */
	{"\xFF", 1, "audio/mp3"},					/* audio mp3 */
	{"\x23\x21\x41\x4D\x52\x0A", 6, "audio/amr"},			/* audio AMR */
	{"\x23\x21\x41\x4D\x52\x2D\x57\x42", 8, "audio/amr-wb"},	/* audio AMR WB */
	{"\x00\x00\x00", 3, "audio/mp4"},				/* audio mp4 */
	{"\x2E\x73\x6E\x64", 4, "audio/au"}				/* audio AU */
};

const char *file_mime_type(const char *filename, const char *buf, int buflen)
{
	unsigned int i;

	/* check for matching magic headers */
	for (i = 0; i < ARRAY_SIZE(mime_types); i++) {

		if (buflen < mime_types[i].magic_len)	/* data is shorter than size of magic */
			continue;

		if (memcmp(buf, mime_types[i].magic, mime_types[i].magic_len) == 0)
			return mime_types[i].mime;
	}

	/* we did not find the MIME type, so return the default (application/octet-stream) */
	return MIME_TYPE_OCTETSTREAM;
}

