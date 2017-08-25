/*
 * Copyright (C)2011 D. R. Commander.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the libjpeg-turbo Project nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* This program demonstrates how to check for the colorspace extension
   capabilities of libjpeg-turbo at both compile time and run time. */

#include <stdio.h>
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
#include "string.h"
#include "stdlib.h"
#include "qdbmp.h"

#ifndef JCS_EXTENSIONS
#define JCS_EXT_RGB 6
#endif
#if !defined(JCS_EXTENSIONS) || !defined(JCS_ALPHA_EXTENSIONS)
#define JCS_EXT_RGBA 12
#endif

static char lasterror[JMSG_LENGTH_MAX] = "No error";

typedef struct _error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf jb;
} error_mgr;

static void my_error_exit(j_common_ptr cinfo)
{
  error_mgr *myerr = (error_mgr *)cinfo->err;
  (*cinfo->err->output_message)(cinfo);
  longjmp(myerr->jb, 1);
}

static void my_output_message(j_common_ptr cinfo)
{
  (*cinfo->err->format_message)(cinfo, lasterror);
}

void hello_kocsog()
{
	int jcs_valid = -1, jcs_alpha_valid = -1;
	struct jpeg_compress_struct cinfo;
	error_mgr jerr;

	printf("libjpeg-turbo colorspace extensions:\n");
#if JCS_EXTENSIONS
	printf("  Present at compile time\n");
#else
  printf("  Not present at compile time\n");
#endif

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	jerr.pub.output_message = my_output_message;

	if(setjmp(jerr.jb)) {
		/* this will execute if libjpeg has an error */
		jcs_valid = 0;
		goto done;
	}

	jpeg_create_compress(&cinfo);
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);
	cinfo.in_color_space = JCS_EXT_RGB;
	jpeg_default_colorspace(&cinfo);
	jcs_valid = 1;

done:
	if (jcs_valid)
		printf("  Working properly\n");
	else
		printf("  Not working properly.  Error returned was:\n    %s\n",
		       lasterror);

	printf("libjpeg-turbo alpha colorspace extensions:\n");
#if JCS_ALPHA_EXTENSIONS
	printf("  Present at compile time\n");
#else
  printf("  Not present at compile time\n");
#endif

	if(setjmp(jerr.jb)) {
		/* this will execute if libjpeg has an error */
		jcs_alpha_valid = 0;
		goto done2;
	}

	cinfo.in_color_space = JCS_EXT_RGBA;
	jpeg_default_colorspace(&cinfo);
	jcs_alpha_valid = 1;

done2:
	if (jcs_alpha_valid)
		printf("  Working properly\n");
	else
		printf("  Not working properly.  Error returned was:\n    %s\n",
		       lasterror);

	jpeg_destroy_compress(&cinfo);
}
/*
* IMAGE DATA FORMATS:
*
* The standard input image format is a rectangular array of pixels, with
* each pixel having the same number of "component" values (color channels).
* Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
* If you are working with color data, then the color values for each pixel
* must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
* RGB color.
*
* For this example, we'll assume that this data structure matches the way
* our application has stored the image in memory, so we can just pass a
* pointer to our image buffer.  In particular, let's say that the image is
* RGB color and is described by:
*/

extern JSAMPLE *image_buffer;   /* Points to large array of R,G,B-order data */
extern int image_height;        /* Number of rows in image */
extern int image_width;         /* Number of columns in image */

int read_JPEG_file(char *filename)
{
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;

	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct _error_mgr jerr;
	/* More stuff */
	FILE *infile;                 /* source file */
	JSAMPARRAY buffer;            /* Output row buffer */
	int row_stride;               /* physical row width in output buffer */

								  /* In this example we want to open the input file before doing anything else,
								  * so that the setjmp() error recovery below can assume the file is open.
								  * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
								  * requires it in order to read binary files.
								  */

	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return 0;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.jb)) {
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

		
	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.txt for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	cinfo.dct_method = JDCT_FLOAT;

	(void)jpeg_start_decompress(&cinfo);

	cinfo.dct_method = JDCT_FLOAT;

	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

	//cinfo.out_color_space = JCS_EXT_RGB;

	UCHAR* shit = (UCHAR*)malloc(row_stride * cinfo.output_height);
	
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	int y = 0;
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could ask for
		* more than one scanline at a time if that's more convenient.
		*/
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		//put_scanline_someplace(buffer[0], row_stride);
		
		UCHAR* destPtr = shit + y*row_stride;
		memcpy(destPtr, buffer[0], row_stride);
		
		y++;
	}

	
	BMP* bmp = BMP_Create(cinfo.output_width, cinfo.output_height, 24);
	BMP_CHECK_ERROR(stderr, -3);

	for (UINT y = 0; y<cinfo.output_height; y++)
	{
		for (UINT x = 0; x < cinfo.output_width; x++)
		{
			UCHAR* pixel = shit + y*row_stride + x;

			UCHAR r = pixel[0];
			UCHAR g = pixel[1];
			UCHAR b = pixel[2];
			BMP_SetPixelRGB(bmp, x, y, r, g, b);
		}
	}

	BMP_WriteFile(bmp, "c://dev//GitHub//_ImageSharp//libjpeg-turbo//_VS2015//sharedlib//Debug//FuckingResult.bmp");

	BMP_CHECK_ERROR(stderr, -3);

	BMP_Free(bmp);
	free(shit);

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	fclose(infile);

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return 1;
}


void WriteMetaData(jpeg_decompress_struct srcinfo, FILE* outputFile)
{
	const short componentCount = (short)srcinfo.num_components;

	fwrite(&componentCount, sizeof(short), 1, outputFile);

	for (JDIMENSION compNum = 0; compNum < srcinfo.num_components; compNum++)
	{
		jpeg_component_info* compInfo = &srcinfo.comp_info[compNum];

		const short widthInBlocks = (short) compInfo->width_in_blocks;
		const short heightInBlocks = (short) compInfo->height_in_blocks;

		printf("compNum: %d, widthInBlocks: %d, heightInBlocks: %d\n", compNum, widthInBlocks, heightInBlocks);

		fwrite(&widthInBlocks, sizeof(short), 1, outputFile);
		fwrite(&heightInBlocks, sizeof(short), 1, outputFile);

		size_t blockRowSize = (size_t) sizeof(JCOEF) * DCTSIZE2 * widthInBlocks;
	}
}

void process_coeffs(struct jpeg_decompress_struct srcinfo, 
	jvirt_barray_ptr* src_coef_arrays,
	const char* outputFileName)
{
	JBLOCKARRAY rowPtrs[MAX_COMPONENTS];

	printf("COMP COUNT: %d\n", srcinfo.num_components);
	
	FILE* outputFile = fopen(outputFileName, "wb");
	if (!outputFile)
	{
		printf("Failed to open %s", outputFileName);
		return;
	}

	printf("writing to: %s\n", outputFileName);

	// Dumping every metadata entry as Int16!
	WriteMetaData(srcinfo, outputFile);

	for (JDIMENSION compNum = 0; compNum < srcinfo.num_components; compNum++) {
		jpeg_component_info* compInfo = &srcinfo.comp_info[compNum];

		const short widthInBlocks = (short)compInfo->width_in_blocks;
		const short heightInBlocks = (short)compInfo->height_in_blocks;
		
		for (JDIMENSION rowNum = 0; rowNum < heightInBlocks; rowNum++) {
			// A pointer to the virtual array of dct values
			rowPtrs[compNum] = ((&srcinfo)->mem->access_virt_barray)((j_common_ptr)&srcinfo, src_coef_arrays[compNum], rowNum, (JDIMENSION)1, FALSE);
			// Loop through the blocks to get the dct values
			
			for (JDIMENSION blockNum = 0; blockNum < widthInBlocks; blockNum++) {
				//...iterate over DCT coefficients

				short* block = rowPtrs[compNum][0][blockNum];
				//printf("B[%d][0]=%d | ", blockNum, block[0]);

				fwrite(block, sizeof(short), 64, outputFile);

				//for (JDIMENSION i = 0; i<DCTSIZE2; i++) {
					//and print them to standard out - one per line			
				//}
			}
			//printf("\n");
		}
	}
	fclose(outputFile);
}

int dump_blocks(const char* inputJpegFileName, const char* outputFileName)
{
	FILE * infile;
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_error_mgr srcerr;

	if ((infile = fopen(inputJpegFileName, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", inputJpegFileName);
		return 0;
	}

	srcinfo.err = jpeg_std_error(&srcerr);
	jpeg_create_decompress(&srcinfo);
	jpeg_stdio_src(&srcinfo, infile);
	(void)jpeg_read_header(&srcinfo, FALSE);

	//coefficients
	jvirt_barray_ptr * src_coef_arrays = jpeg_read_coefficients(&srcinfo);
	process_coeffs(srcinfo, src_coef_arrays, outputFileName);

	jpeg_destroy_decompress(&srcinfo);
	fclose(infile);
	return 1;
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("usage:");
		printf("dump-jpeg-coeffs <input.jpg> [output.dctdump]");
		return -1;
	}
	char* inputFileName = argv[1];
	char outputFileName[256];
	outputFileName[0] = '\0';

	if (argc == 2)
	{
		strcpy(outputFileName, inputFileName);
		strcat(outputFileName, ".dctdump");
	}
	else
	{
		strcpy(outputFileName, argv[2]);
	}
	
	if (!dump_blocks(inputFileName, outputFileName))
	{
		printf("Failed to open %s", inputFileName);
	}

	/*char foo[42];
	scanf(foo);*/
    return 0;
}
