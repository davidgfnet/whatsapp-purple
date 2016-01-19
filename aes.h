
#ifndef AES_IMPL_H__
#define AES_IMPL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

#define AES_BLOCK_SIZE 16

#define KEYLENGTH(keybits) ((keybits)/8)
#define RKLENGTH(keybits)  ((keybits)/8+28)
#define NROUNDS(keybits)   ((keybits)/32+6)

typedef struct {
	int nrounds;
	uint32_t rk[RKLENGTH(256)];
} AES_KEY;

#define AES_ENCRYPT 1
#define AES_DECRYPT 0

void AES_set_encrypt_key(const unsigned char *key, const int length, AES_KEY * state);
void AES_set_decrypt_key(const unsigned char *key, const int length, AES_KEY * state);
void AES_cbc_encrypt(const unsigned char *in, unsigned char *out, size_t length, const AES_KEY *key, unsigned char *ivec, const int enc);

#ifdef __cplusplus
}
#endif

#endif

