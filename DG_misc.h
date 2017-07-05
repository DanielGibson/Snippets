/*
 * Misc. useful public domain functions.
 * Assumes a C99 compatible Compiler (gcc and clang tested) or MS Visual C++
 *
 * Copyright (C) 2015-2016 Daniel Gibson
 *
 * Homepage: https://github.com/DanielGibson/Snippets/
 *
 * Do:
 *   #define DG_MISC_IMPLEMENTATION
 * before you include this file in *one* of your .c/.cpp files.
 * Ideally, this is the first thing you #include in that .c/.cpp file.
 * Or second: #include <stdarg.h> before this to get DG_vsnprintf()
 *
 * The code uses assertions. You can #define DG_MISC_ASSERT(condition, message)
 * to your own liking to overwrite (or deactivate) them, if you don't,
 * the default is #define DG_MISC_ASSERT assert( (condition) && (message) )
 *
 * You can #define DG_MISC_DEF if you want to prepend anything to the
 * function signatures (like "static", "inline", "__declspec(dllexport)", ...)
 * Example: #define DG_MISC_DEF static inline
 *
 * Supported Microsoft Visual C++ Versions:
 *  Tested MSVC 2013 and 2010 (it just works for them), and MSVC 6.0, which works
 *  with little changes: you need to "typedef unsigned int uintptr_t;" before
 *  #including this file (because back then they didn't have uintptr_t) and you
 *  need to comment out the call to _vscprintf() in DG_vsnprintf(), because that
 *  function wasn't supported either. In that case, DG_(v)snprintf() will
 *  return -1 (instead of the needed buffer length), if the buffer was too small.
 *  Might be similar for other MSVC versions between 6 and 2010, I didn't test.
 *  I'm not gonna test MSVC6 regularly, maybe just use a newer compiler :-P
 *
 * License:
 *  This software is dual-licensed to the public domain and under the following
 *  license: you are granted a perpetual, irrevocable license to copy, modify,
 *  publish, and distribute this file as you see fit.
 *  No warranty implied; use at your own risk.
 *
 * So you can do whatever you want with this code, including copying it
 * (or parts of it) into your own source.
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */

#ifndef __DG_MISC_H__
#define __DG_MISC_H__

// this allows you to prepend stuff to function signatures, e.g. "static"
#ifndef DG_MISC_DEF
// by default it's empty
#define DG_MISC_DEF
#endif // DG_MISC_DEF

// for size_t:
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// returns the full path to your executable, including the executable itself
// returns empty string on error
DG_MISC_DEF const char* DG_GetExecutablePath(void);

// returns the full path to the directory your executable is in,
// incl. (back)slash, but without the executable itself
// returns empty string on error
DG_MISC_DEF const char* DG_GetExecutableDir(void);

// returns the filename of the executable, without the path
// basically, DG_GetExecutableDir() concatenated with DG_GetExecutableFilename()
//   is DG_GetExecutablePath()
// returns empty string on error
DG_MISC_DEF const char* DG_GetExecutableFilename(void);

// copy up to n chars of str into a new string which is guaranteed to be
// '\0'-terminated.
// Needs to be free'd with free(), returns NULL if allocation failed
DG_MISC_DEF char* DG_strndup(const char* str, size_t n);

// copies up to dstsize-1 bytes from src to dst and ensures '\0' termination
// returns the number of chars that would be written into a big enough dst
// buffer without the terminating '\0'
DG_MISC_DEF size_t DG_strlcpy(char* dst, const char* src, size_t dstsize);

// appends src to the existing null-terminated(!) string in dst while making
// sure that dst will not contain more than dstsize bytes incl. terminating '\0'
// returns the number of chars that would be written into a big enough dst
// buffer without the terminating '\0'
DG_MISC_DEF size_t DG_strlcat(char* dst, const char* src, size_t dstsize);

// See also https://www.freebsd.org/cgi/man.cgi?query=strlcpy&sektion=3
// for details on strlcpy and strlcat.

// search for needle in haystack, like strstr(), but for binary data.
// haystack and needle are buffers with given lengths in byte and will be
// interpreted as unsigned char* for comparison. the comparison used is like memcmp()
// returns the address of the first match, or NULL if it wasn't found
DG_MISC_DEF void* DG_memmem(const void* haystack, size_t haystacklen,
                            const void* needle, size_t needlelen);

// search for last occurence of needle in haystack, like DG_memmem() but backwards.
// haystack and needle are buffers with given lengths in byte and will be
// interpreted as unsigned char* for comparison. the comparison used is like memcmp()
// returns the address of the last match, or NULL if it wasn't found
DG_MISC_DEF void* DG_memrmem(const void* haystack, size_t haystacklen,
                             const void* needle, size_t needlelen);

// returns the last occurence byte c in buf (searching backwards from buf[buflen-1] on)
// like strrchr(), but for binary data, or like memchr() but backwards.
// returns NULL if c wasn't found in buf.
DG_MISC_DEF void* DG_memrchr(const void* buf, unsigned char c, size_t buflen);

// search for last occurence of needle in haystack, like strstr() but backwards.
// also like DG_memrmem(), but for '\0'-terminated strings.
// returns the address of the last match, or NULL if it wasn't found
DG_MISC_DEF char* DG_strrstr(const char* haystack, const char* needle);

// like strtok, but threadsafe - saves the context in context.
// so do char* ctx; foo = DG_strtok_r(bar, " \t", &ctx);
// See http://linux.die.net/man/3/strtok_r for more details
DG_MISC_DEF char* DG_strtok_r(char* str, const char* delim, char** context);

// on many platforms (incl. windows and freebsd) this implementation is faster
// than the libc's strnlen(). on others (linux/glibc, OSX) it just calls the
// ASM-optimized strnlen() provided by the libc.
// I didn't bother to use a #define because strnlen() (in contrast to strlen())
// is no compiler-builtin (at least for GCC) anyway.

// returns the length of the '\0'-terminated string s in chars
// if there is no '\0' in the first n chars, returns n
DG_MISC_DEF size_t DG_strnlen(const char* s, size_t n);

#ifndef _WIN32
// other libc implementations have a fast strlen... use a #define so compilers
// recognizes strlen and can optimize/use a builtin

// returns the length of the '\0'-terminated string s in chars
#define DG_strlen strlen

// other libc implementors than Microsoft implemented (v)snprintf() properly.
#define DG_snprintf snprintf
#define DG_vsnprintf vsnprintf

#else // it *is* _WIN32, DG_strlen(), DG_snprintf() and DG_vsnprintf(), implemented as functions on win32

// returns the length of the '\0'-terminated string s in chars
DG_MISC_DEF size_t DG_strlen(const char* s);

// a snprintf() implementation that is conformant to C99 by ensuring
// '\0'-termination of dst and returning the number of chars (without
// terminating '\0') that would've been written to a big enough buffer
// However, it still might do microsoft-specific printf formatting
//   int DG_snprintf(char *dst, size_t size, const char *format, ...);

// several different cases to do printf format checking with different compilers for DG_snprintf()
#if defined(_MSC_VER) && _MSC_VER >= 1400 // MSVC2005 and newer have an annotation. only used in /analyze builds.
	#include <CodeAnalysis\SourceAnnotations.h>
	DG_MISC_DEF int DG_snprintf(char *dst, size_t size,
	             [SA_FormatString(Style="printf")] const char *format, ...);
#elif defined(__GNUC__) // mingw or similar, checking with GCC attribute
	DG_MISC_DEF int DG_snprintf(char *dst, size_t size,
	             const char *format, ...) __attribute__ ((format (printf, 3, 4)));
#else // some other compiler, no printf format checking
	DG_MISC_DEF int DG_snprintf(char *dst, size_t size, const char *format, ...);
#endif // _MSC_VER or __GNUC__

#ifdef va_start // it's a macro and defined if the user #included stdarg.h

// a vsnprintf() implementation that is conformant to C99 by ensuring
// '\0'-termination of dst and returning the number of chars (without
// terminating '\0') that would've been written to a big enough buffer
// However, it still might do microsoft-specific printf formatting
DG_MISC_DEF int DG_vsnprintf(char *dst, size_t size, const char *format, va_list ap);

#endif // va_start

#endif // _WIN32

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __DG_MISC_H__

// ****************************************************************************
// under here: implementations

#ifdef DG_MISC_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif

// DG_MISC_NO_GNU_SOURCE can be used to enforce usage of our own memrchr() and memmem() functions
// on Linux - mostly relevant for testing (they should be faster than my implementation)

#ifndef DG_MISC_ASSERT
#define DG_MISC_ASSERT(cond, msg) assert( (cond) && (msg) )
#include <assert.h>
#endif

#include <string.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h> // INT_MAX, maybe PATH_MAX

// for uintptr_t:
#ifndef _MSC_VER
#include <stdint.h>
#endif

#if defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <unistd.h> // readlink(), amongst others
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h> // for sysctl() to get path to executable
#endif

#ifdef _WIN32
#include <windows.h> // GetModuleFileNameA()
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifndef PATH_MAX
// this is mostly for windows. windows has a MAX_PATH = 260 #define, but allows
// longer paths anyway.. this might not be the maximum allowed length, but is
// hopefully good enough for realistic usecases
#define PATH_MAX 4096
#define _DG__DEFINED_PATH_MAX
#endif

static void DG__SetExecutablePath(char* exePath)
{
	// !!! this assumes that exePath can hold PATH_MAX chars !!!

#ifdef _WIN32

	DWORD len = GetModuleFileNameA(NULL, exePath, PATH_MAX);
	if(len <= 0 || len == PATH_MAX)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

#elif defined(__linux) || defined(__NetBSD__) || defined(__OpenBSD__)

	// all the platforms that have /proc/$pid/exe or similar that symlink the
	// real executable - basiscally Linux and the BSDs except for FreeBSD which
	// doesn't enable proc by default and has a sysctl() for this
	char buf[PATH_MAX] = {0};
#ifdef __linux
	snprintf(buf, sizeof(buf), "/proc/%d/exe", getpid());
#else // the BSDs
	snprintf(buf, sizeof(buf), "/proc/%d/file", getpid());
#endif
	// readlink() doesn't null-terminate!
	int len = readlink(buf, exePath, PATH_MAX-1);
	if (len <= 0)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}
	else
	{
		exePath[len] = '\0';
	}

#elif defined(__FreeBSD__)

	// the sysctl should also work when /proc/ is not mounted (which seems to
	// be common on FreeBSD), so use it..
	int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	size_t len = PATH_MAX-1;
	int ret = sysctl(name, sizeof(name)/sizeof(name[0]), exePath, &len, NULL, 0);
	if(ret != 0)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

#elif defined(__APPLE__)

	uint32_t bufSize = PATH_MAX;
	if(_NSGetExecutablePath(exePath, &bufSize) != 0)
	{
		// WTF, PATH_MAX is not enough to hold the path?
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

	// TODO: realpath() ?
	// TODO: no idea what this is if the executable is in an app bundle

#else

#error "Unsupported Platform!" // feel free to add implementation for your platform and send me a patch

#endif
}

DG_MISC_DEF const char* DG_GetExecutablePath(void)
{
	static char exePath[PATH_MAX] = {0};

	if(exePath[0] != '\0') return exePath;

	// the following code should only be executed at the first call of this function
	DG__SetExecutablePath(exePath);

	return exePath;
}

DG_MISC_DEF const char* DG_GetExecutableDir(void)
{
	static char exeDir[PATH_MAX] = {0};

	if(exeDir[0] != '\0') return exeDir;

	// the following code should only be executed at the first call of this function
	const char* exePath = DG_GetExecutablePath();

	if(exePath == NULL || exePath[0] == '\0') return exeDir;

	DG_strlcpy(exeDir, exePath, PATH_MAX);

	// cut off executable name
	char* lastSlash = strrchr(exeDir, '/');
#ifdef _WIN32
	char* lastBackSlash = strrchr(exeDir, '\\');
	if(lastSlash == NULL || lastBackSlash > lastSlash) lastSlash = lastBackSlash;
#endif // _WIN32

	if(lastSlash != NULL) lastSlash[1] = '\0'; // cut off after last (back)slash

	return exeDir;
}

DG_MISC_DEF const char* DG_GetExecutableFilename(void)
{
	static const char* exeName = "";
	if(exeName[0] != '\0') return exeName;

	// the following code should only be executed at the first call of this function
	const char* exePath = DG_GetExecutablePath();

	if(exePath == NULL || exePath[0] == '\0') return exeName;

	// cut off executable name
	const char* lastSlash = strrchr(exePath, '/');
#ifdef _WIN32
	const char* lastBackSlash = strrchr(exePath, '\\');
	if(lastSlash == NULL || lastBackSlash > lastSlash) lastSlash = lastBackSlash;
#endif // _WIN32

	if(lastSlash != NULL && lastSlash[1] != '\0')
	{
		// the filename starts after the last (back)slash
		exeName = lastSlash+1;
	}

	return exeName;
}

DG_MISC_DEF char* DG_strndup(const char* str, size_t n)
{
	DG_MISC_ASSERT(str != NULL, "Don't call DG_strndup() with NULL!");

	size_t len = DG_strnlen(str, n);
	char* ret = (char*)malloc(len+1); // need one more byte for terminating 0
	if(ret != NULL)
	{
		memcpy(ret, str, len);
		ret[len] = '\0';
	}
	return ret;
}

// Notes on DG_strlcat() and DG_strlcpy():
// My implementations use (DG_)strlen(), (DG_)strnlen() and memcpy(), which are
// usually heavily optimized. Thus they're faster than the BSD strl*
// implementations in most cases (those iterate the strings bytewise themselves)
// - unless the function call overhead is higher than iterating the bytes,
//   which only happens for very short strings and is negligible.
//  Furthermore the speedup depends on how well optimized your libc is.)
// Note that strlcat() is *not* the same as strncat(), which takes the max
//  number of bytes that should be appended, which is mostly useless.
// And that strlcpy() is *not* the same as strncpy(), which fills up the unused
//  part of the buffer with '\0', but doesn't guarantee '\0'-termination if the
//  buffer isn't big enough.. and thus is pretty useless.

DG_MISC_DEF size_t DG_strlcpy(char* dst, const char* src, size_t dstsize)
{
	DG_MISC_ASSERT(src && dst, "Don't call strlcpy with NULL arguments!");
	size_t srclen = DG_strlen(src);

	if(dstsize != 0)
	{
		size_t numchars = dstsize-1;

		if(srclen < numchars) numchars = srclen;

		memcpy(dst, src, numchars);
		dst[numchars] = '\0';
	}
	return srclen;
}

DG_MISC_DEF size_t DG_strlcat(char* dst, const char* src, size_t dstsize)
{
	DG_MISC_ASSERT(src && dst, "Don't call strlcat with NULL arguments!");

	size_t dstlen = DG_strnlen(dst, dstsize);
	size_t srclen = DG_strlen(src);

	DG_MISC_ASSERT(dstlen != dstsize, "dst must contain null-terminated data with strlen < dstsize!");

	// TODO: dst[dstsize-1] = '\0' to ensure null-termination and make wrong dstsize more obvious?

	if(dstsize > 1 && dstlen < dstsize-1)
	{
		size_t numchars = dstsize-dstlen-1;

		if(srclen < numchars) numchars = srclen;

		memcpy(dst+dstlen, src, numchars);
		dst[dstlen+numchars] = '\0';
	}

	return dstlen + srclen;
}

DG_MISC_DEF void* DG_memmem(const void* haystack, size_t haystacklen,
                            const void* needle, size_t needlelen)
{
	DG_MISC_ASSERT((haystack != NULL || haystacklen == 0)
			&& (needle != NULL || needlelen == 0),
			"Don't pass NULL into DG_memmem(), unless the corresponding len is 0!");

#if defined(_GNU_SOURCE) && !defined(DG_MISC_NO_GNU_SOURCE)
	// glibc has a very optimized version of this, use that instead
	return memmem(haystack, haystacklen, needle, needlelen);
#else
	unsigned char* h = (unsigned char*)haystack;
	unsigned char* n = (unsigned char*)needle;

	if(needlelen == 0) return (void*)haystack; // this is what glibc does..
	if(haystacklen < needlelen) return NULL; // also handles haystacklen == 0

	if(needlelen == 1) return (void*)memchr(haystack, n[0], haystacklen);

	// TODO: knuth-morris-pratt or boyer-moore or something like that might be a lot faster.

	// the byte after the last byte needle could start at so it'd still fit into haystack
	unsigned char* afterlast = h + haystacklen - needlelen + 1;
	// haystack length up to afterlast
	size_t hlen_for_needle_start = afterlast - h;
	int n0 = n[0];
	unsigned char* n0candidate = (unsigned char*)memchr(h, n0, hlen_for_needle_start);

	while(n0candidate != NULL)
	{
		if(memcmp(n0candidate+1, n+1, needlelen-1) == 0)
		{
			return (void*)n0candidate;
		}

		++n0candidate; // go on searching one byte after the last n0candidate
		hlen_for_needle_start = afterlast - n0candidate;
		n0candidate = (unsigned char*)memchr(n0candidate, n0, hlen_for_needle_start);
	}

	return NULL; // not found

#endif // _GNU_SOURCE
}


DG_MISC_DEF void* DG_memrchr(const void* buf, unsigned char c, size_t buflen)
{
	DG_MISC_ASSERT(buf != NULL, "Don't pass NULL into DG_memrchr()!");
#if defined(_GNU_SOURCE) && !defined(DG_MISC_NO_GNU_SOURCE)
	// glibc has a very optimized version of this, use that instead
	return (void*)memrchr(buf, c, buflen);
#else

	// TODO: this could use a variation of the trick used in DG_strnlen(), as described
	//       on https://graphics.stanford.edu/~seander/bithacks.html#ValueInWord

	unsigned char* cur = (unsigned char*)buf + buflen;
	const unsigned char* b = (const unsigned char*)buf;
	while(cur > b) // aborts immediately if buflen == 0
	{
		--cur;
		if(*cur == c) return cur;
	}

	return NULL;
#endif // _GNU_SOURCE
}

DG_MISC_DEF void* DG_memrmem(const void* haystack, size_t haystacklen,
                             const void* needle, size_t needlelen)
{
	DG_MISC_ASSERT((haystack != NULL || haystacklen == 0)
			&& (needle != NULL || needlelen == 0),
			"Don't pass NULL into DG_memrmem(), unless the corresponding len is 0!");

	unsigned char* h = (unsigned char*)haystack;
	unsigned char* n = (unsigned char*)needle;

	if(needlelen == 0) return (void*)(h+haystacklen); // this is kinda analog to DG_memmem()'s behavior
	if(haystacklen < needlelen) return NULL; // also handles haystacklen == 0

	if(needlelen == 1) return (void*)DG_memrchr(haystack, n[0], haystacklen);

	// TODO: knuth-morris-pratt or boyer-moore or something like that might be a lot faster.

	// the byte after the last byte needle could start at so it'd still fit into haystack
	unsigned char* afterlast = h + haystacklen - needlelen + 1;
	// haystack length up to afterlast
	size_t hlen_for_needle_start = afterlast - h;
	int n0 = n[0];
	unsigned char* n0candidate = (unsigned char*)DG_memrchr(h, n0, hlen_for_needle_start);

	while(n0candidate != NULL)
	{
		if(memcmp(n0candidate+1, n+1, needlelen-1) == 0)
		{
			return (void*)n0candidate;
		}

		// now n0candidate is the char *after* the end of the part of
		// the string we still care about
		hlen_for_needle_start = n0candidate - h;
		n0candidate = (unsigned char*)DG_memrchr(h, n0, hlen_for_needle_start);
	}

	return NULL; // not found
}

DG_MISC_DEF char* DG_strrstr(const char* haystack, const char* needle)
{
	size_t hLen = DG_strlen(haystack);
	size_t nLen = DG_strlen(needle);
	return (char*)DG_memrmem(haystack, hLen, needle, nLen);
}

/* 
 * public domain strtok_r() by Charlie Gordon
 * see http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 * and http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 * with a fix from Fletcher T. Penney, also in public domain,
 * see https://github.com/fletcher/MultiMarkdown-4/blob/master/strtok.c
 */
DG_MISC_DEF char* DG_strtok_r(char* str, const char* delim, char** context)
{
	DG_MISC_ASSERT(context && delim, "Don't call DG_strtok_r() with delim or context set to NULL!");
	DG_MISC_ASSERT(str || *context, "Don't call DG_strtok_r() with *context and str both set to NULL!");

#if !defined(DG_MISC_NO_GNU_SOURCE) && !defined(_WIN32)

	// I think every interesting platform except for Windows supports strtok_r
	// (if not, add it above with "&& !defined(_OTHER_CRAPPY_PLATFORM)")
	return strtok_r(str, delim, context);

#else // Windows

	// I don't wanna use MSVC's strtok_s(), because C11 defines a function with
	// that name but a totally different signature.. and it's not supported
	// by old MSVC versions.
	char* ret;

	if (str == NULL) str = *context;

	if (str == NULL) return NULL;

	str += strspn(str, delim);

	if (*str == '\0') return NULL;

	ret = str;

	str += strcspn(str, delim);

	if (*str) *str++ = '\0';

	*context = str;

	return ret;
#endif // _WIN32
}

DG_MISC_DEF size_t DG_strnlen(const char* s, size_t n)
{
	DG_MISC_ASSERT(s != NULL, "Don't call DG_strnlen() with NULL!");

#if (defined(__GLIBC__) || defined(__APPLE__)) && !defined(DG_MISC_NO_GNU_SOURCE)
	// glibc has a very optimized version of this, use that instead
	// apple also seems to have optimized ASM code, see
	// http://www.opensource.apple.com/source/Libc/Libc-1044.1.2/x86_64/string/
	return strnlen(s, n);
#else
	// at least microsoft and freebsd seem to use a naive strnlen() without
	// any tricks which is usually slower than this

	// uses a trick from https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
	// (and probably in 1000 other places) to decide whether sizeof(uintptr_t) bytes
	// contain a '\0' or not. without branching, with relatively few instructions
	// that trick only works (at least in the way I've implemented it) with 32bit and 64bit systems
	DG_MISC_ASSERT((sizeof(uintptr_t) == 4 || sizeof(uintptr_t) == 8), "DG_strnlen() only works for 32bit and 64bit systems!");
	// these magic numbers are used for the trick:
#if defined(_MSC_VER) && !defined(_WIN64)
	// some older MSVC versions (tested 6.0) don't support ULL suffixes.. not sure
	// when they started supporting them, but for non-64bit windows these constants works
	static const uintptr_t magic1 = 0x01010101uL;
	static const uintptr_t magic2 = 0x80808080uL;
#else // better compilers/64bit win
	static const uintptr_t magic1 = (sizeof(uintptr_t) == 4) ? 0x01010101uL : 0x0101010101010101uLL;
	static const uintptr_t magic2 = (sizeof(uintptr_t) == 4) ? 0x80808080uL : 0x8080808080808080uLL;
#endif

	// let's get the empty buffer special case out of the way...
	if(n==0) return 0;

	// s aligned to the next word boundary
	uintptr_t s_alnI = ((uintptr_t)s + sizeof(uintptr_t) - 1) & ~(sizeof(uintptr_t)-1);
	const uintptr_t* s_aln = (uintptr_t *)s_alnI;
	const char* s_last = (const char*)(~((uintptr_t)0)); // highest possible address (all bits are 1)

	if(n < (uintptr_t)(s_last - s))
	{
		// adding n won't overflow, so it's safe to do that now.
		// otherwise s_last will remain to be the highest possible address
		// Note: this is a bit cryptic, but according to http://lwn.net/Articles/278137/
		// a check like "if(s+n-1 < s)" might be optimized away by many compilers
		
		// now it points to the last but one byte in s (if no byte up to and
		// including this one is '\0', we'll return n)
		s_last = s+n-1;
	}

	// check bytes between s and s_aln
	const char* cur = s;
	for( ; cur < (const char*)s_aln; ++cur)
	{
		if(*cur == '\0')
		{
			if(cur > s_last) return n;

			return cur - s;
		}
	}

	for( ; (const char*)s_aln <= s_last; ++s_aln)
	{
		// the aforementioned trick
		uintptr_t m1 = *s_aln - magic1;
		uintptr_t m2 = (~(*s_aln)) & magic2;

		if(m1 & m2)
		{
			cur = (const char*)s_aln;
			size_t i;
			for(i=0; i<sizeof(uintptr_t); ++i)
			{
				if(cur[i] == '\0')
				{
					size_t ret = cur + i - s;
					return (ret < n) ? ret : n;
				}
			}
		}
	}

	return n;

#endif // __GLIBC__
}

#ifdef _WIN32
/* value of _MSC_VER macro for different MSVC versions,
 * so I don't have to google that over and over again
 * (from http://sourceforge.net/p/predef/wiki/Compilers/#microsoft-visual-c)
 * Visual C++   _MSC_VER
 * 15.3 (2017)   1911 // "Visual Studio 2017 Version 15.3 Preview"
 * 15.0 (2017)   1910
 * 14.0 (2015)   1900
 * 12.0 (2013)   1800
 * 11.0 (2012)   1700
 * 10.0 (2010)   1600
 * 9.0  (2008)   1500
 * 8.0  (2005)   1400 // first with 64bit support?
 * 7.1  (2003)   1310
 * 7.0           1300
 * 6.0           1200
 * 5.0           1100
 * 4.2           1020
 * 4.0           1000
 * 3.0           900
 * 1.0           800
 */


DG_MISC_DEF size_t DG_strlen(const char* s)
{
	// glibc's strlen() is *fucking* fast (with custom ASM), Apple also has custom ASM,
	// freebsd uses the same trick as DG_strnlen() and is slightly faster...
	// but Microsoft's strlen() is slower than DG_strnlen(), so use that for Windows.
	// I don't feel like duplicating all that strnlen() code, so let's just pass
	// the max. possible length (until the highest address a pointer can store)
	static const char* maxaddr = (const char*)(~((uintptr_t)0));
	return DG_strnlen(s, maxaddr - s);
}

DG_MISC_DEF int DG_vsnprintf(char *dst, size_t size, const char *format, va_list ap)
{
	DG_MISC_ASSERT(format, "Don't pass a NULL format into DG_vsnprintf()!");
	// TODO: assert(size <= INT_MAX && "Don't pass a size > INT_MAX to DG_vsnprintf()!"); ??
	//       after all, we're supposed to return the number of bytes written.. as an int.

	int ret = -1;
	if(dst != NULL && size > 0)
	{
#if defined(_MSC_VER) && _MSC_VER >= 1400
		// I think MSVC2005 introduced _vsnprintf_s().
		// this shuts up _vsnprintf() security/deprecation warnings.
		ret = _vsnprintf_s(dst, size, _TRUNCATE, format, ap);
#else
		ret = _vsnprintf(dst, size, format, ap);
		dst[size-1] = '\0'; // ensure '\0'-termination
#endif
	}

	if(ret == -1)
	{
		// _vsnprintf() returns -1 if the output is truncated
		// it's also -1 if dst or size were NULL/0, so the user didn't want to write
		// we want to return the number of characters that would've been
		// needed, though.. fortunately _vscprintf() calculates that.
		ret = _vscprintf(format, ap);

		// NOTE: on ancient MSVC versions you may get an error because
		//  _vscprintf() is not supported.. I don't know when it started being
		//  supported and I don't want to silently make this function incorrect
		//  anyway.. feel free to comment out that line yourself, -1 will be
		//  returned in case of a too short buffer then.
		//  At least it'll still be '\0'-terminated.
	}

	return ret;
}

DG_MISC_DEF int DG_snprintf(char *dst, size_t size, const char *format, ...)
{
	DG_MISC_ASSERT(format, "Don't pass a NULL format into DG_snprintf()!");

	int ret = 0;

	va_list argptr;
	va_start( argptr, format );

	ret = DG_vsnprintf(dst, size, format, argptr);

	va_end( argptr );

	return ret;
}
#endif // _WIN32

#ifdef _DG__DEFINED_PATH_MAX
#undef PATH_MAX
#undef _DG__DEFINED_PATH_MAX
#endif // _DG__DEFINED_PATH_MAX

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DG_MISC_IMPLEMENTATION
