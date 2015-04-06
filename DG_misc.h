/*
 * Misc. useful public domain functions.
 * Assumes a C99 compatible Compiler or MSVC.
 * 
 * Copyright (C) 2015 Daniel Gibson
 *
 * Do:
 *   #define DG_MISC_IMPLEMENTATION
 * before you include this file in *one* of your .c/.cpp files.
 * Ideally, this is the first thing you #include in that .c/.cpp file.
 * Or second: #include <stdarg.h> before this to get DG_vsnprintf()
 *
 * License:
 *  This software is in the public domain. Where that dedication is not
 *  recognized, you are granted a perpetual, irrevocable license to copy
 *  and modify this file however you want.
 *  No warranty implied; use at your own risk.
 */

#ifndef __DG_MISC_H__
#define __DG_MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

// returns the full path to your executable, including the executable itself
// returns empty string on error
const char* DG_GetExecutablePath(void);

// returns the full path to the directory your executable is in,
// incl. (back)slash, but without the executable itself
// returns empty string on error
const char* DG_GetExecutableDir(void);

// returns the filename of the executable, without the path
// basically, DG_GetExecutableDir() concatenated with DG_GetExecutableFilename()
//   is DG_GetExecutablePath()
// returns empty string on error
const char* DG_GetExecutableFilename(void);

// copy up to n chars of str into a new string which is guaranteed to be
// '\0'-terminated. 
// Needs to be free'd with free(), returns NULL if allocation failed
char* DG_strndup(const char* str, size_t n);

// copies up to dstsize-1 bytes from src to dst.
// returns the number of chars that would be written into a big enough dst
// buffer without the terminating '\0'
size_t DG_strlcpy(char* dst, const char* src, size_t dstsize);

// appends src to the existing null-terminated(!) string in dst while making
// sure that dst will not contain more than dstsize bytes incl. terminating '\0'
// returns the number of chars that would be written into a big enough dst
// buffer without the terminating '\0'
size_t DG_strlcat(char* dst, const char* src, size_t dstsize);

// See also https://www.freebsd.org/cgi/man.cgi?query=strlcpy&sektion=3
// for details on strlcpy and strlcat.
// My implementations use strlen(), strnlen() and memcpy() from the C std lib,
// which are usually heavily optimized. Thus they're faster than the BSD strl*
// implementations in most cases (those iterate the strings bytewise themselves)
// (unless the function call overhead is higher than iterating the bytes,
//  which only happens for very short strings and is negligible.
//  Furthermore the speedup depends on how well optimized your libc is.)
// Note that strlcat() *not* the same as strncat(), which takes the max
//  number of bytes that should be appended, which is mostly useless.
// And that strlcpy() is *not* the same as strncpy(), which fills up the unused
//  part of the buffer with '\0', but doesn't guarantee '\0'-termination if the
//  buffer isn't big enough.. and thus is pretty useless.

// search for needle in haystack, like strstr(), but for binary data.
// haystack and needle are buffers with given lengths in byte and will be
// interpreted as unsigned char* for comparison. the comparison used is like memcmp()
// returns the address of the first match, or NULL if it wasn't found
void* DG_memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen);

// returns the last occurence byte c in buf (searching backwards from buf[buflen-1] on)
// like strrchr(), but for binary data, or like memchr() but backwards.
// returns NULL if c wasn't found in buf.
void* DG_memrchr(const void* buf, unsigned char c, size_t buflen);

#ifndef _WIN32

// other libc implementors than Microsoft implemented (v)snprintf() properly.
#define DG_snprintf snprintf
#define DG_vsnprintf vsnprintf

#else // it *is* _WIN32, DG_snprintf() and DG_vsnprintf(), implemented as functions on win32

// a snprintf() implementation that is conformant to C99 by ensuring
// '\0'-termination of dst and returning the number of chars (without
// terminating '\0') that would've been written to a big enough buffer
// However, it still might do microsoft-specific printf formatting
//   int DG_snprintf(char *dst, int size, const char *format, ...);

// several different cases to do printf format checking with different compilers for DG_snprintf()
#if defined(_MSC_VER) && _MSC_VER >= 1400 // MSVC2005 and newer have an annotation. only used in /analyze builds.
	#include <CodeAnalysis\SourceAnnotations.h>
	int DG_snprintf(char *dst, int size, [SA_FormatString(Style="printf")] const char *format, ...);
#elif defined(__GNUC__) // mingw or similar, checking with GCC attribute
	int DG_snprintf(char *dst, int size, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
#else // some other compiler, no printf format checking
	int DG_snprintf(char *dst, int size, const char *format, ...);
#endif // _MSC_VER or __GNUC__

#ifdef va_start // it's a macro and defined if the user #included stdarg.h

// a vsnprintf() implementation that is conformant to C99 by ensuring
// '\0'-termination of dst and returning the number of chars (without
// terminating '\0') that would've been written to a big enough buffer
// However, it still might do microsoft-specific printf formatting
int DG_vsnprintf(char *dst, int size, const char *format, va_list ap);

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
#if !defined(_GNU_SOURCE) && !defined(DG_MISC_NO_GNU_SOURCE)
#define _GNU_SOURCE // for glibc versions of memrchr() and memmem()
#define _DG__DEFINED_GNU_SOURCE
#endif // _GNU_SOURCE

#include <string.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if defined(__linux) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include <unistd.h> // readlink(), amongst others
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h> // for sysctl() to get path to executable
#endif

#ifdef _WIN32
#include <Winbase.h> // GetModuleFileNameA()
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

#ifndef PATH_MAX
// this is mostly for windows. windows has a MAX_PATH = 260 #define, but allows
// longer paths anyway.. this might not be the maximum allowed length, but is
// hopefully good enough for realistic usecases
#define PATH_MAX 4096
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
	int len = readlink(buf, exePath, PATH_MAX);
	if (len <= 0)
	{
		// an error occured, clear exe path
		exePath[0] = '\0';
	}

#elif defined(__FreeBSD__)

	// the sysctl should also work when /proc/ is not mounted (which seems to
	// be common on FreeBSD), so use it..
	int name[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	int ret = sysctl(name, sizeof(name)/sizeof(name[0]), exePath, PATH_MAX-1, NULL, 0);
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

const char* DG_GetExecutablePath(void)
{
	static char exePath[PATH_MAX] = {0};
	
	if(exePath[0] != '\0') return exePath;
	
	// the following code should only be executed at the first call of this function
	DG__SetExecutablePath(exePath);

	return exePath;
}

const char* DG_GetExecutableDir(void)
{
	static char exeDir[PATH_MAX] = {0};
	
	if(exeDir[0] != '\0') return exeDir;
	
	// the following code should only be executed at the first call of this function
	const char* exePath = DG_GetExecutablePath();
	
	if(exePath == NULL || exePath[0] == '\0') return exeDir;
	
	strcpy(exeDir, exePath); // both are PATH_MAX, so this should be safe.
	
	// cut off executable name
	char* lastSlash = strrchr(exeDir, '/');
#ifdef _WIN32
	char* lastBackSlash = strrchr(exeDir, '\\');
	if(lastSlash == NULL || lastBackSlash > lastSlash) lastSlash = lastBackSlash;
#endif // _WIN32
	
	if(lastSlash != NULL) lastSlash[1] = '\0'; // cut off after last (back)slash
	
	return exeDir;
}

const char* DG_GetExecutableFilename(void)
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

char* DG_strndup(const char* str, size_t n)
{
	assert(str != NULL && "Don't call DG_strndup() with NULL!");
	// strnlen() is not a standard C function, but even MSVC supports it (at least) since MSVC2005
	// if that turns out to be a problem report that and I might provide a DG_strnlen()
	size_t len = strnlen(str, n);
	char* ret = (char*)malloc(len+1); // need one more byte for terminating 0
	if(ret != NULL)
	{
		memcpy(ret, str, len);
		ret[len] = '\0';
	}
	return ret;
}

size_t DG_strlcpy(char* dst, const char* src, size_t dstsize)
{
	assert(src && dst && "Don't call strlcpy with NULL arguments!");
	size_t srclen = strlen(src);
	
	if(dstsize != 0)
	{
		size_t numchars = dstsize-1;
		
		if(srclen < numchars) numchars = srclen;
		
		memcpy(dst, src, numchars);
		dst[numchars] = '\0';
	}
	return srclen;
}

size_t DG_strlcat(char* dst, const char* src, size_t dstsize)
{
	assert(src && dst && "Don't call strlcat with NULL arguments!");
	// strnlen() is not a standard C function, but even MSVC supports it (at least) since MSVC2005
	size_t dstlen = strnlen(dst, dstsize);
	size_t srclen = strlen(src);
	
	assert(dstlen != dstsize && "dst must contain null-terminated data with strlen < dstsize!");
	
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

void* DG_memmem(const void* haystack, size_t haystacklen, const void* needle, size_t needlelen)
{
	assert((haystack != NULL || haystacklen == 0)
		&& (needle != NULL || needlelen == 0)
		&& "Don't pass NULL into DG_memmem(), unless the corresponding len is 0!");
	
#if defined(__GLIBC__) && !defined(DG_MISC_NO_GNU_SOURCE)
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
	unsigned char* n0candidate = (unsigned char*) memchr(h, n0, hlen_for_needle_start);
	
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
	
#endif // __GLIBC__
}


void* DG_memrchr(const void* buf, unsigned char c, size_t buflen)
{
	assert(buf != NULL && "Don't pass NULL into DG_memrchr()!");
#if defined(__GLIBC__) && !defined(DG_MISC_NO_GNU_SOURCE)
	// glibc has a very optimized version of this, use that instead
	return (void*)memrchr(buf, c, buflen);
#else
	if(buflen > 0)
	{
		unsigned char* cur = (unsigned char*)buf + buflen - 1;
		do {
			if(*cur == c) return cur;
			--cur;
		} while(cur != buf);
		
		if(*cur == c) return cur;
	}
	
	return NULL;
#endif // __GLIBC__
}

#ifdef _WIN32
int DG_vsnprintf(char *dst, int size, const char *format, va_list ap)
{
	assert(format && "Don't pass a NULL format into DG_vsnprintf()!");
	assert(size >= 0 && "Don't pass a negative size to DG_vsnprintf()!");
	
	int ret = -1;
	if(dst != NULL && size > 0)
	{
		ret = _vsnprintf(dst, size, format, ap);
		dst[size-1] = '\0'; // ensure '\0'-termination
	}
	
	if(ret == -1)
	{
		// _vsnprintf() returns -1 if the output is truncated
		// it's also -1 if dst or size were NULL/0, so the user didn't want to write
		// we want to return the number of characters that would've been
		// needed, though.. fortunately _vscprintf() calculates that.
		ret = _vscprintf(format, ap);
	}
	
	return ret;
}

int DG_snprintf(char *dst, int size, const char *format, ...)
{
	assert(format && "Don't pass a NULL format into DG_snprintf()!");
	assert(size >= 0 && "Don't pass a negative size to DG_snprintf()!");
	
	int ret = 0;
	
	va_list argptr;
	va_start( argptr, format );
	
	ret = DG_vsnprintf(dst, size, format, argptr);
	
	va_end( argptr );
	
	return ret;
}
#endif // _WIN32

#ifdef _DG__DEFINED_GNU_SOURCE
// make sure _GNU_SOURCE doesn't leak into files #including this, unless they
// already #defined it themselves
#undef _GNU_SOURCE
#undef _DG__DEFINED_GNU_SOURCE
#endif // _DG__DEFINED_GNU_SOURCE

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DG_MISC_IMPLEMENTATION
