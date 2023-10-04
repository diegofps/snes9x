/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifdef HAVE_LIBPNG
#include <png.h>
#endif
#include "snes9x.h"
#include "memmap.h"
#include "display.h"
#include "screenshot.h"
#include "stdio.h"

#include <filesystem>
namespace fs = std::filesystem;
FILE * movie_file = nullptr;
long movie_frame_counter = 0;
u_char * movie_frame = nullptr;
int movie_frame_width = 0;
int movie_frame_height = 0;

std::string S9xGetHardFilenameInc(std::string e, enum s9x_getdirtype dirtype, int counter)
{
	fs::path rom_filename(Memory.ROMFilename);

	fs::path filename_base(S9xGetDirectory(dirtype));
	filename_base /= rom_filename.filename();
	filename_base.replace_extension(".frames");
	fs::create_directory(filename_base);
	filename_base /= "frame";

	fs::path new_filename;

	if (e[0] != '.')
		e = "." + e;
    
	std::string new_extension = std::to_string(counter);

	while (new_extension.length() < 6)
		new_extension = std::string("0") + new_extension;
	new_extension += e;

	new_filename = filename_base;
	new_filename.replace_extension(new_extension);

	printf("%s\n", new_filename.c_str());
	
	return new_filename;
}

void S9xDoRawMovie(int const width, int const height)
{
	if (movie_file == nullptr)
	{
		std::string fname = S9xGetFilenameInc(".movie", SCREENSHOT_DIR);
		movie_file = fopen(fname.c_str(), "w");
	}

	if (movie_file == nullptr)
	{
		printf("Can't save frame, invalid movie_file\n");
		return;
	}

	if (movie_frame == nullptr)
	{
		movie_frame = new u_char[width * height * 3];
		movie_frame_width = width;
		movie_frame_height = height;
	}

	if (movie_frame_height != height || movie_frame_width != width)
	{
		printf("Can't save movie, invalid frame size\n");
		return;
	}

	uint16 *screen = GFX.Screen;
	u_char * rowpix = movie_frame;

	for (int y=0;y<height;y++,screen+=GFX.RealPPL)
		for (int x = 0; x < width; x++)
		{
			uint32	r, g, b;

			DECOMPOSE_PIXEL(screen[x], r, g, b);

			*(rowpix++) = r;
			*(rowpix++) = g;
			*(rowpix++) = b;
		}
	
	fwrite(movie_frame, sizeof(u_char), width * height * 3, movie_file);

	if (movie_frame_counter++ % 60 == 0)
	{
		printf("Movie file flushed\n");
		fflush(movie_file);
	}
}

bool8 S9xDoScreenshot (int width, int height)
{
	Settings.TakeScreenshot = FALSE;

#ifdef HAVE_LIBPNG
	FILE		*fp;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_color_8	sig_bit;
	int			imgwidth, imgheight;

	// std::string fname = S9xGetFilenameInc(".png", SCREENSHOT_DIR);
	std::string fname = S9xGetHardFilenameInc(".png", SCREENSHOT_DIR, movie_frame_counter++);

	fp = fopen(fname.c_str(), "wb");
	if (!fp)
	{
		S9xMessage(S9X_ERROR, 0, "Failed to take screenshot.");
		return (FALSE);
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		fclose(fp);
		remove(fname.c_str());
		S9xMessage(S9X_ERROR, 0, "Failed to take screenshot.");
		return (FALSE);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
		fclose(fp);
		remove(fname.c_str());
		S9xMessage(S9X_ERROR, 0, "Failed to take screenshot.");
		return (FALSE);
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		remove(fname.c_str());
		S9xMessage(S9X_ERROR, 0, "Failed to take screenshot.");
		return (FALSE);
	}

	imgwidth  = width;
	imgheight = height;

	if (Settings.StretchScreenshots == 1)
	{
		if (width > SNES_WIDTH && height <= SNES_HEIGHT_EXTENDED)
			imgheight = height << 1;
	}
	else if (Settings.StretchScreenshots == 2)
	{
		if (width  <= SNES_WIDTH)
			imgwidth  = width  << 1;
		if (height <= SNES_HEIGHT_EXTENDED)
			imgheight = height << 1;
	}
	png_set_compression_level(png_ptr, 3);
	png_init_io(png_ptr, fp);

	png_set_IHDR(png_ptr, info_ptr, imgwidth, imgheight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	sig_bit.red   = 5;
	sig_bit.green = 5;
	sig_bit.blue  = 5;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	png_set_shift(png_ptr, &sig_bit);

	png_write_info(png_ptr, info_ptr);

	png_set_packing(png_ptr);

	png_byte	*row_pointer = new png_byte[png_get_rowbytes(png_ptr, info_ptr)];
	uint16		*screen = GFX.Screen;

	for (int y = 0; y < height; y++, screen += GFX.RealPPL)
	{
		png_byte	*rowpix = row_pointer;

		for (int x = 0; x < width; x++)
		{
			uint32	r, g, b;

			DECOMPOSE_PIXEL(screen[x], r, g, b);

			*(rowpix++) = r;
			*(rowpix++) = g;
			*(rowpix++) = b;

			if (imgwidth != width)
			{
				*(rowpix++) = r;
				*(rowpix++) = g;
				*(rowpix++) = b;
			}
		}

		png_write_row(png_ptr, row_pointer);
		if (imgheight != height)
			png_write_row(png_ptr, row_pointer);
	}

	delete [] row_pointer;

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);

	fprintf(stderr, "%s saved.\n", fname.c_str());

	std::string base = "Saved screenshot " + S9xBasename(fname);
	S9xMessage(S9X_INFO, 0, base.c_str());

	return (TRUE);
#else
	fprintf(stderr, "Screenshot support not available (libpng was not found at build time).\n");
	return (FALSE);
#endif
}
