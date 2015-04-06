# Snippets

Some standalone source files that don't need separate repositories.

| File                          | Description    |
|-------------------------------|----------------|
| **DG_misc.h**                 | A public domain single-header C library with some useful functions to get the path/dir/name of the current executable and misc. string operations that are not available on all platforms  |
| **sdl2_scancode_to_dinput.h** | One static C array that maps SDL2 scancodes to Direct Input keynums (values of those DIK_* constants) - also public domain.                                                                        |
| **ImgToC.c**                  | Commandline tool converting images to .c files with a struct containing the image data. Same format as Gimp's "Export as .c" feature. Needs [stb_image.h](https://github.com/nothings/stb/) |

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

// returns the last occurence byte c in buf. Like strrchr() for binary data.
void* DG_memrchr(const void* buf, unsigned char c, size_t buflen);

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
