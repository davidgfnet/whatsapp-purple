
#ifndef IMG_UTIL_H_
#define IMG_UTIL_H_

#ifdef __cplusplus
extern "C" {
#endif


void imgProfile(const unsigned char * data, unsigned int size, void ** out, int * outlen, int dimensions);
void imgThumbnail(const unsigned char * data, unsigned int size, void ** out, int * outlen, int maxdimensions);

#ifdef __cplusplus
}
#endif

#endif

