
#include <stdio.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <string.h>

// Borreowed from my opengx repo, which borrowed it from mesa i think

void scale_internal(int components, int widthin, int heightin,const unsigned char *datain,
			   int widthout, int heightout,unsigned char *dataout) {
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float xpercent,ypercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    halfconvx = convx/2;
    halfconvy = convy/2;
    for (i = 0; i < heightout; i++) {
		y = convy * (i+0.5);
		if (heightin > heightout) {
			highy = y + halfconvy;
			lowy = y - halfconvy;
		} else {
			highy = y + 0.5;
			lowy = y - 0.5;
		}
		for (j = 0; j < widthout; j++) {
			x = convx * (j+0.5);
			if (widthin > widthout) {
				highx = x + halfconvx;
				lowx = x - halfconvx;
			} else {
				highx = x + 0.5;
				lowx = x - 0.5;
			}

			/*
			** Ok, now apply box filter to box that goes from (lowx, lowy)
			** to (highx, highy) on input data into this pixel on output
			** data.
			*/
			totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
			area = 0.0;

			y = lowy;
			yint = floor(y);
			while (y < highy) {
				yindex = (yint + heightin) % heightin;
				if (highy < yint+1) {
					ypercent = highy - y;
				} else {
					ypercent = yint+1 - y;
				}

				x = lowx;
				xint = floor(x);

				while (x < highx) {
					xindex = (xint + widthin) % widthin;
					if (highx < xint+1) {
						xpercent = highx - x;
					} else {
						xpercent = xint+1 - x;
					}

					percent = xpercent * ypercent;
					area += percent;
					temp = (xindex + (yindex * widthin)) * components;
					for (k = 0; k < components; k++) {
						totals[k] += datain[temp + k] * percent;
					}

					xint++;
					x = xint;
				}
				yint++;
				y = yint;
		    }

			temp = (j + (i * widthout)) * components;
			for (k = 0; k < components; k++) {
				/* totals[] should be rounded in the case of enlarging an RGB
				 * ramp when the type is 332 or 4444
				 */
				dataout[temp + k] = (totals[k]+0.5)/area;
			}
		}
    }
}


int imgResize(void * inb, size_t ins, void ** outb, unsigned long * outs, int desiredsize) {
	int i;

	// init compress & decompress struct
	struct jpeg_decompress_struct in;
	struct jpeg_compress_struct out;
	struct jpeg_error_mgr jInErr, jOutErr;

	in.err = jpeg_std_error(&jInErr);
	out.err = jpeg_std_error( &jOutErr);

	jpeg_create_decompress(&in);
	jpeg_create_compress(&out);

	jpeg_mem_src(&in, inb, ins);
	jpeg_read_header(&in, TRUE);
	jpeg_start_decompress(&in);

	*outb = malloc(ins);
	*outs = ins;

	jpeg_mem_dest(&out, outb, outs);

	int width = in.output_width;
	int height = in.output_height;
	int bytesPerPixel = in.num_components;
	float scalingw = ((float)desiredsize) / width;

	int destWidth  = (int) (width  * scalingw);
	int destHeight = (int) (height * scalingw);

	out.image_width = desiredsize;
	out.image_height = desiredsize;
	out.input_components = bytesPerPixel;
	out.in_color_space = JCS_RGB;

	jpeg_set_defaults(&out);
	jpeg_set_quality(&out, 50, TRUE);
	jpeg_start_compress(&out, TRUE);

	char* inbuffer = malloc(width*height*3);
	void* outbuffer = malloc(desiredsize*desiredsize*3);
	memset(outbuffer, 0, desiredsize*desiredsize*3);

	// If more height than width skip some lines
	int src_skip = (destHeight > desiredsize) ? (height-width)/2 : 0;

	for (i = src_skip; i < height-src_skip; i++) {
		JSAMPROW inRowPointer[1];
		inRowPointer[0] = &inbuffer[(i-src_skip)*width*3];

		jpeg_read_scanlines(&in, inRowPointer, 1);
	}

	int dst_skip = (destHeight < desiredsize) ? (desiredsize-destHeight)/2 : 0;

	if (destHeight > desiredsize)
		scale_internal(3, width, width, inbuffer, desiredsize, desiredsize, outbuffer);
	else
		scale_internal(3, width, height, inbuffer, desiredsize, destHeight, &outbuffer[dst_skip*desiredsize*3]);

	for (i = 0; i < desiredsize; i++) {
		JSAMPROW inRowPointer[1] = { &outbuffer[i*desiredsize*3] };
		jpeg_write_scanlines(&out, &inRowPointer, 1);
	}

	free(inbuffer);
	free(outbuffer);

	//jpeg_finish_decompress(&in);
	jpeg_destroy_decompress(&in);

	jpeg_finish_compress(&out);
	jpeg_destroy_compress(&out);

	return 1;
}


