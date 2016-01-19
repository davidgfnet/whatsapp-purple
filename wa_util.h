
/*
 * WhatsApp API implementation in C++ for libpurple.
 * Written by David Guillen Fandos (david@davidgf.net) based 
 * on the sources of WhatsAPI PHP implementation.
 *
 * Share and enjoy!
 *
 */

#ifndef WA_UTIL_H__
#define WA_UTIL_H__

#ifdef ENABLE_OPENSSL
// Use OpenSSL functions
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#else
// OpenSSL replacements, either custom or libpurple
#include <cipher.h>
#include "aes.h"
unsigned char *MD5(const unsigned char *d, int n, unsigned char *md);
int PKCS5_PBKDF2_HMAC_SHA1(const char *pass, int passlen, const unsigned char *salt, int saltlen, int iter, int keylen, unsigned char *out);
unsigned char *SHA1(const unsigned char *d, int n, unsigned char *md);

#endif

#ifdef __cplusplus
#include <iostream>
std::string md5hex(std::string target);
std::string md5raw(std::string target);
std::string SHA256_file_b64(const char *filename);
std::string getpreview(const char *filename);
#endif

const char *file_mime_type(const char *filename, const char *buf, int buflen);

#ifdef DEBUG
#define DEBUG_PRINT(a) std::clog << a << std::endl;
#else
#define DEBUG_PRINT(a)
#endif

std::string tohex(const char *t, int l);

#endif
