# Snippets

Some standalone source files that don't deserve their own repositories.

| File                          | Description    |
|-------------------------------|----------------|
| [**DG_misc.h**](/DG_misc.h) | A public domain single-header C library with some useful functions to get the path/dir/name of the current executable and misc. string operations that are not available on all platforms - [***List of Functions***]( #list-of-functions-in-dg_misch) |
| [**SDL_stbimage.h**](/SDL_stbimage.h) | A public domain single-header C library for converting images to [SDL2](http://libsdl.org) `SDL_Surface*` using [stb_image.h](https://github.com/nothings/stb) - [***List of Functions***]( #list-of-function-in-sdl_stbimageh) |
| [**sdl2_scancode_to_dinput.h**](/sdl2_scancode_to_dinput.h) | One static C array that maps SDL2 scancodes to Direct Input keynums (values of those DIK_* constants) - also public domain. |
| [**ImgToC.c**](/ImgToC.c) | Commandline tool converting images to .c files with a struct containing the image data. Same format as Gimp's "Export as .c" feature. Needs [stb_image.h](https://github.com/nothings/stb/) |

## List of functions in **DG_misc.h**

The "DG_" prefix is not just to please my big ego, but mostly to (hopefully) avoid name collisions.

The `DG_GetExecutable*()` functions have been tested on Linux, Windows and FreeBSD.  
They *should* also work with NetBSD, OpenBSD and Mac OS X (not sure what they do with .app bundles, though).  
Adding more platforms shouldn't be hard, patches/pull requests are welcome :-)

```c
// get full path to your executable, including the executable itself
const char* DG_GetExecutablePath(void);

// get full path to the directory your executable is in, without executable itself
const char* DG_GetExecutableDir(void);

// get filename of the executable, without the path
const char* DG_GetExecutableFilename(void);

// copy up to n chars of str into a new string, guaranteed to be'\0'-terminated.
char* DG_strndup(const char* str, size_t n);

// copies up to dstsize-1 bytes from src to dst and ensures '\0' termination
size_t DG_strlcpy(char* dst, const char* src, size_t dstsize);

// appends src to the existing null-terminated(!) string in dst
size_t DG_strlcat(char* dst, const char* src, size_t dstsize);

// See also https://www.freebsd.org/cgi/man.cgi?query=strlcpy&sektion=3
// for details on strlcpy() and strlcat().

// search for needle in haystack, like strstr(), but for binary data.
void* DG_memmem(const void* haystack, size_t haystacklen,
                const void* needle, size_t needlelen);

// search for last occurence of needle in haystack, like DG_memmem() but backwards.
void* DG_memrmem(const void* haystack, size_t haystacklen,
                 const void* needle, size_t needlelen);

// returns the last occurence byte c in buf. Like strrchr() for binary data.
void* DG_memrchr(const void* buf, unsigned char c, size_t buflen);

// search for last occurence of needle in haystack, like strstr() but backwards.
// also like DG_memrmem(), but for '\0'-terminated strings.
// returns the address of the last match, or NULL if it wasn't found
char* DG_strrstr(const char* haystack, const char* needle);

// reentrant (threadsafe) version of strtok(), saves its progress into context.
char* DG_strtok_r(char* str, const char* delim, char** context);

// returns the length of the '\0'-terminated string s.
// might be faster than default strnlen(), otherwise it will use default strnlen()
size_t DG_strnlen(const char* s, size_t n);

// calculates length of s. might be faster than default strlen()
// (otherwise it will use default strlen)
size_t DG_strlen(const char* s);

// a snprintf() implementation that is conformant to C99 by ensuring
// '\0'-termination of dst and returning the number of chars (without
// terminating '\0') that would've been written to a big enough buffer
// (for non-windows platforms it's just a #define to snprintf())
int DG_snprintf(char *dst, size_t size, const char *format, ...);

// the same for vsnprintf() (only enabled if you #include <stdarg.h> first!)
int DG_vsnprintf(char *dst, size_t size, const char *format, va_list ap);
```

## List of function in **SDL_stbimage.h**

```c
// loads the image file at the given path into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Surface* STBIMG_Load(const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Surface* STBIMG_LoadFromMemory(const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Surface from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Surface* STBIMG_Load_RW(SDL_RWops* src, int freesrc);


// loads an image file into a RGB(A) SDL_Surface from a SDL_RWops (src)
// - without using SDL_RWseek(), for streams that don't support or are slow
//   at seeking. It reads everything into a buffer and calls STBIMG_LoadFromMemory()
// You should probably only use this if you *really* have performance problems
//  because of seeking or your src doesn't support  SDL_RWseek(), but SDL_RWsize()
// src must at least support SDL_RWread() and SDL_RWsize()
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Surface* STBIMG_Load_RW_noSeek(SDL_RWops* src, int freesrc);


// Creates an SDL_Surface* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// If freeWithSurface is SDL_TRUE, SDL_FreeSurface() will free the pixelData
//  you passed with SDL_free() - NOTE that you should only do that if pixelData
//  was allocated with SDL_malloc(), SDL_calloc() or SDL_realloc()!
// Returns NULL on error (in that case pixelData won't be freed!),
//  use SDL_GetError() to get more information.
SDL_Surface* STBIMG_CreateSurface(unsigned char* pixelData, int width, int height,
                                  int bytesPerPixel, SDL_bool freeWithSurface);


// creates stbi_io_callbacks and userdata to use stbi_*_from_callbacks() directly,
//  especially useful to use SDL_RWops with stb_image, without using SDL_Surface
// src must be readable and seekable!
// Returns SDL_FALSE on error (SDL_GetError() will give you info), else SDL_TRUE
// NOTE: If you want to use src twice (e.g. for info and load), remember to rewind
//       it by seeking back to its initial position and resetting out->atEOF to 0
//       inbetween the uses!
SDL_bool STBIMG_stbi_callback_from_RW(SDL_RWops* src, STBIMG_stbio_RWops* out);

typedef struct {
	SDL_RWops* src;
	stbi_io_callbacks stb_cbs;
	int atEOF; // defaults to 0; 1: reached EOF or error on read, 2: error on seek
} STBIMG_stbio_RWops;
```
