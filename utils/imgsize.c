#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <jpeglib.h>
#include <gif_lib.h>
#include <png.h>

static void try_jpeg(const char *fname)
{
	FILE *fp = fopen(fname, "r");
	if (!fp) {
		perror(fname);
		exit(1);
	}

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);

	printf("%dx%d jpeg\n", cinfo.image_width, cinfo.image_height);
}

static int try_gif(const char *fname, int no_fail)
{
#if GIFLIB_MAJOR == 4 /* guess */
	GifFileType *gft = DGifOpenFileName(fname);

	if (!gft) {
		if (no_fail)
			return 1;

		fprintf(stderr, "%s: Unable to open gif file (%d)\n",
				fname, GifLastError());
		exit(1);
	}
#else
	int error;
	GifFileType *gft = DGifOpenFileName(fname, &error);

	if (!gft) {
		if (no_fail)
			return 1;

		fprintf(stderr, "%s: %s\n", fname, GifErrorString(error));
		exit(1);
	}
#endif

	printf("%dx%d gif\n", gft->SWidth, gft->SHeight);
	return 0;
}

#define png_abort(str) do {							\
		if (fp) fclose(fp);							\
		if (no_fail) return 1;						\
		fprintf(stderr, "%s: %s\n", fname, str);	\
		exit(1);									\
	} while (0)

static int try_png(const char *fname, int no_fail)
{
	FILE *fp = fopen(fname, "r");
	if (!fp)
		png_abort("Unable to open file");

	uint8_t header[8];
	int n = fread(header, 1, sizeof(header), fp);
	if (n < 8 || png_sig_cmp(header, 0, n))
		png_abort("Not a PNG file");

	png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
		png_abort("OOM");

	png_infop info_ptr;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		png_abort("OOM");

	if (setjmp(png_jmpbuf(png_ptr)))
		png_abort("I/O error");

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	printf("%ux%u png\n",
		   (int)png_get_image_width(png_ptr, info_ptr),
		   (int)png_get_image_height(png_ptr, info_ptr));

	return 0;
}

static void try_all(const char *fname)
{
	if (try_gif(fname, 1))
		if (try_png(fname, 1))
			try_jpeg(fname);
}

void usage(int rc)
{
	puts("usage:\timgsize [-gjh] image_file\n"
		 "where:\t-g assume gif\n"
		 "\t-j assume jpeg\n"
		 "\t-h this help"
		 "\nSupports gif and jpeg. Outputs widthxheight.");
	exit(rc);
}

#define DO_GIF  1
#define DO_JPEG 2
#define DO_PNG  4

int main(int argc, char *argv[])
{
	int c, flags = 0;
	char *fname;

	while ((c = getopt(argc, argv, "ghjp")) != EOF)
		switch (c) {
		case 'g': flags = DO_GIF; break;
		case 'h': usage(0);
		case 'j': flags = DO_JPEG; break;
		case 'p': flags = DO_PNG; break;
		default: usage(1);
		}

	if (optind == argc) {
		puts("I need a file name.");
		exit(1);
	}
	fname = argv[optind];

	switch (flags) {
	case DO_GIF: try_gif(fname, 0); break;
	case DO_JPEG: try_jpeg(fname); break;
	case DO_PNG: try_png(fname, 0); break;
	default: try_all(fname); break;
	}

	return 0;
}
