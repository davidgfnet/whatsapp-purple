
#include <FreeImage.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

void imgProfile(const unsigned char * data, unsigned int size, void ** out, int * outlen, int dimensions) {
	FreeImage_Initialise(0);

	FIMEMORY * inimgmem = FreeImage_OpenMemory(data, size);
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(inimgmem, size);
	FIBITMAP * inimg = FreeImage_LoadFromMemory(fif, inimgmem, 0);
	
	// Create a squared version of the img
	unsigned width = FreeImage_GetWidth(inimg);
	unsigned height = FreeImage_GetHeight(inimg);

	double scalew = dimensions / ((double)width);
	double scaleh = dimensions / ((double)height);
	double scale  = (width > height) ? scalew : scaleh;
	unsigned nwidth  = round(width  * scale);
	unsigned nheight = round(height * scale);

	int left = 0, top = 0;
	if (nwidth > nheight)
		top = (nwidth-nheight)/2;
	else
		left = (nheight-nwidth)/2;

	FIBITMAP * outimg_ = FreeImage_Rescale(inimg, nwidth, nheight, FILTER_CATMULLROM);
	FIBITMAP * outimg = FreeImage_Allocate(dimensions, dimensions, 24, 0,0,0);
	FreeImage_Paste(outimg, outimg_, left, top, 256);

	FIMEMORY * outimgmem = FreeImage_OpenMemory(0,0);
	FreeImage_SaveToMemory(FIF_JPEG, outimg, outimgmem, JPEG_QUALITYNORMAL);

	*outlen = FreeImage_TellMemory(outimgmem);
	*out = malloc(*outlen);
	unsigned char *tbuf;
	FreeImage_AcquireMemory(outimgmem, &tbuf, (unsigned*)outlen);
	memcpy(*out, tbuf, *outlen);

	FreeImage_Unload(outimg);
	FreeImage_Unload(outimg_);
	FreeImage_Unload(inimg);
	FreeImage_CloseMemory(inimgmem);
	FreeImage_CloseMemory(outimgmem);
}



