/*
 * convert image files to .c files that contain a struct, same format as gimp's .c export
 * needs stb_image.h from https://github.com/nothings/stb
 *
 * The name of the struct will be "img_outfilename" where outfilename
 * is the filename of the output file  (just the filename, not the whole path,
 * without file extension and chars not allowed in C identifiers are replaced by '_')
 * Images with an alpha channel will be written as RGBA, images without one as RGB.
 *
 * Usage: ImgToC <imgfilename> [outfilename]
 *   eg:  ImgToC test.png
 *           will create test.c from test.png
 *        ImgToC /path/to/bla.tga /other/path/to/bla.h
 *           will create /other/oath/to/bla.h from bla.tga
 *
 * if you #define DEBUG_DISPLAY_IMAGE and link against libsdl2, you get a really
 * crappy image viewer that displays the image to be converted. mainly useful for
 * debugging ("is the output broken because the input file wasn't loaded correctly?")
 *
 * Can be built with "gcc ImgToC.c -o ImgToC" (or use clang instead of gcc).
 * Building with MSVC (as console application) is probably possible, but untested.
 *
 * (C) 2015 Daniel Gibson
 *
 * Homepage: https://github.com/DanielGibson/Snippets/
 *
 * License:
 *  This software is in the public domain. Where that dedication is not
 *  recognized, you are granted a perpetual, irrevocable license to copy
 *  and modify this file however you want.
 *  No warranty implied; use at your own risk.
 */

#if 0 // Example of such a struct:
 static const struct {
 	unsigned int width;
 	unsigned int height;
 	unsigned int bytes_per_pixel; /* 3:RGB, 4:RGBA */
 	unsigned char pixel_data[64 * 64 * 4 + 1];
 } img_outfilename = {
 	64, 64, 4,
 	"\216\301\332\216\301\332\277a9\354\0\362\t\345\362 \367\345\362\367"
 	"... etc, a lot more lines following for all the data ..."
 };
#endif // 0

#include <stdio.h>
#include <limits.h> // PATH_MAX

#ifndef PATH_MAX
// should be defined by limits.h, but maybe it's not on windows, not sure...
#define PATH_MAX 4096
#endif

#define STBI_NO_LINEAR // don't need HDR stuff
#define STBI_NO_HDR
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

const char* progName = "imgToC";

static void printUsage()
{
	eprintf("Usage: %s <imgname> [outfilename]\n", progName);
	eprintf(" e.g.: %s test.png\n", progName);
	eprintf("       %s /path/to/bla.tga /other/path/to/bla.h\n", progName);
}

struct image
{
	unsigned char* data;
	int w;
	int h;
	int format; // 3: RGB, 4: RGBA
};

static struct image loadImage(const char* imgFileName)
{
	struct image ret = {0};
	
	int inforet = stbi_info(imgFileName, &ret.w, &ret.h, &ret.format);
	if(!inforet)
	{
		eprintf("ERROR: Couldn't load image file %s: %s!\n", imgFileName, stbi_failure_reason());
		exit(1);
	}
	
	int bppToUse = 4;
	// no alpha => use RGB, else use RGBA
	if(ret.format == 1 || ret.format == 3) bppToUse = 3;
	
	ret.data = stbi_load(imgFileName, &ret.w, &ret.h, &ret.format, bppToUse);
	if(ret.data == NULL)
	{
		eprintf("ERROR: Couldn't load image file %s: %s!\n", imgFileName, stbi_failure_reason());
		exit(1);
	}
	ret.format = bppToUse;
	
	return ret;
}

#ifdef DEBUG_DISPLAY_IMAGE // make sure the image is loaded correctly by displaying it in an SDL2 window. needs SDL2.

#include <SDL.h>
static void displayImage(struct image img)
{
	Uint32 rmask, gmask, bmask, amask;
	// ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use a void* for the data,
	// but it's really treated as endian-specific Uint32* ...
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (img.format == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else // little endian, like x86
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (img.format == 3) ? 0 : 0xff000000;
#endif

	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom((void*)img.data, img.w, img.h,
						img.format*8, img.format*img.w,
						rmask, gmask, bmask, amask);
	
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window* win = SDL_CreateWindow(progName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
									   640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	
	SDL_Surface* winSurf = SDL_GetWindowSurface(win);

	Uint32 grey = SDL_MapRGB(winSurf->format, 127, 127, 127);

	SDL_FillRect(winSurf, NULL, grey);
	
	SDL_BlitSurface(surf, NULL, winSurf, NULL);
	SDL_UpdateWindowSurface(win);
	
	while(1)
	{
		SDL_Delay(200);
		SDL_Event ev;
		while(SDL_PollEvent(&ev))
		{
			switch(ev.type)
			{
				case SDL_QUIT: goto end;
					break;
				
				case SDL_KEYDOWN:
					if(ev.key.keysym.sym == SDLK_q) goto end;
					break;
				default:
					break;
			}
		}
		winSurf = SDL_GetWindowSurface(win);
		SDL_FillRect(winSurf, NULL, grey);
		SDL_BlitSurface(surf, NULL, winSurf, NULL);
		SDL_UpdateWindowSurface(win);
	}
	
end:
	SDL_FreeSurface(surf);
	SDL_DestroyWindow(win);
	SDL_Quit();
}
#endif // DEBUG_DISPLAY_IMAGE

static void writeStructHeader(FILE* out, struct image img, const char* structName)
{
	fputs("static const struct {\n", out);
	fputs("\tunsigned int width;\n", out);
	fputs("\tunsigned int height;\n", out);
	fputs("\tunsigned int bytes_per_pixel; /* 3:RGB, 4:RGBA */\n", out); // I don't support 2:RGB16
	// the + 1 is for the implicit terminating \0 in the string literal used to initialize pixel_data
	fprintf(out, "\tunsigned char pixel_data[%d * %d * %d + 1];\n", img.w, img.h, img.format);
	fprintf(out, "} %s = {\n", structName);
	fprintf(out, "\t%d, %d, %d,\n", img.w, img.h, img.format);
}

static int addOctalEscapeToFile(FILE* out, int b)
{
	int ret = fprintf(out, "\\%o", b);
	return ret;
}

static int addByteToFile(FILE* out, unsigned char b)
{
	int ret = 0;
	if(b >= ' ' && b <= '~')
	{
		// printable ascii chars
		switch(b)
		{
			case '\"': // all the chars that need \ escaping
			case '\'':
			case '\\':
			case '?': // avoid trigraphs...
				fputc('\\', out);
				fputc(b, out);
				ret = 2;
				break;
			case '0': // escape potential octal-numbers to avoid ambigueties with non-printable escapes
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				ret = addOctalEscapeToFile(out, b);
				break;
			default: // normal printable char that doesn't need escaping
				fputc(b, out);
				ret = 1;
				break;
		}
	}
	else
	{
		ret = 2; // assume this for \0, \a etc
		switch(b)
		{
			case '\0': fputs("\\0", out); break;
			case '\a': fputs("\\a", out); break;
			case '\b': fputs("\\b", out); break;
			case '\t': fputs("\\t", out); break;
			case '\n': fputs("\\n", out); break;
			case '\v': fputs("\\v", out); break;
			case '\f': fputs("\\f", out); break;
			case '\r': fputs("\\r", out); break;
			default:
				ret = addOctalEscapeToFile(out, b);
				break;
		}
	}
	
	return ret;
}

static void writeStructData(FILE* out, struct image img)
{
	int i;
	int lineChars = 0;
	int numBytes = img.w * img.h * img.format;
	for(i=0; i<numBytes; ++i)
	{
		if(lineChars == 0)
		{
			fputs("\t\"", out);
		}
		lineChars += addByteToFile(out, img.data[i]);
		if(lineChars >= 70)
		{
			// if the line incl "" and tab has around 80 display-chars (8 for tab) or more, start new line
			fputs("\"\n", out);
			lineChars = 0;
		}
	}
	if(lineChars != 0)
	{
		// if we didn't just add the closing " and newline, do it now.
		fputs("\"\n", out);
	}
}

static void writeCfile(struct image img, const char* outFile)
{
	// structName should be the outfilename (just filename, without parent directories)
	// but without the extension.. so for "/bla/foo/dings.c" we want the struct to be called "img_dings"
	const char* fileNameStart = strrchr(outFile, '/');
	if(fileNameStart == NULL)
	{
		fileNameStart = strrchr(outFile, '\\');
	}
	if(fileNameStart == NULL)
	{
		fileNameStart = outFile;
	}
	else
	{
		++fileNameStart; // skip the (back)slash
	}
	
	char structName[256] = {0};
	const char* prefix = "img_";
	int prefixLen = strlen(prefix);
	strcpy(structName, prefix);
	int i;
	for(i=0; i < sizeof(structName)-prefixLen-1 && fileNameStart[i] != '.' && fileNameStart[i] != '\0'; ++i)
	{
		char c = fileNameStart[i];
		if( ( c >= '0' && c <= '9')
		  || (c >= 'A' && c <= 'Z')
		  || (c >= 'a' && c <= 'z')
		  || c == '_')
		{
			// those chars are legal in C identifiers..
			structName[i+prefixLen] = c;
		}
		else
		{
			// .. the others are not, replace with '_'
			structName[i+prefixLen] = '_';
		}
	}
	structName[i+prefixLen] = '\0';
	
#ifdef _WIN32
	FILE* out = fopen(outFile, "wt");
#else
	FILE* out = fopen(outFile, "w");
#endif
	
	if(out == NULL)
	{
		eprintf("ERROR: Couldn't open output file %s!\n", outFile);
		return;
	}
	
	writeStructHeader(out, img, structName);

	writeStructData(out, img);
	
	fprintf(out, "};\n");
	fclose(out);
	
	eprintf("Successfully wrote %s\n", outFile);
}

int main(int argc, char** argv)
{
	progName = argv[0];
	if(argc < 2)
	{
		printUsage();
		exit(1);
	}
	
	const char* fileName = argv[1];
	
	struct image img = loadImage(fileName);
	
#ifdef DEBUG_DISPLAY_IMAGE
	displayImage(img);
#endif
	
	char outFileBuf[PATH_MAX];
	const char* outFile = outFileBuf;
	if(argc > 2)
	{
		outFile = argv[2];
	}
	else
	{
		strncpy(outFileBuf, fileName, PATH_MAX);
		outFileBuf[PATH_MAX-1] = '\0';
		
		char* ext = strrchr(outFileBuf, '.');
		if(ext == NULL)
		{
			eprintf("ERROR: Image file %s has no file extension that could be replaced with .c!\n", fileName);
			exit(1);
		}
		ext[1] = 'c';
		ext[2] = '\0';
		// now outFileBuf contains fileName, with the extension changed to .c
	}
	
	writeCfile(img, outFile);
	
	stbi_image_free(img.data);
	return 0;
}
