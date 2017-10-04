/*
 * (C) 2017 Daniel Gibson
 *
 * Some crossplatform file-handling functions.
 *
 * All filename strings are in UTF-8 (on Windows they're converted from/to WCHAR
 * pseudo UTF-16 before calling the corresponding functions, otherwise they're
 * used as is, assuming that everyone else uses UTF-8)
 *
 * LICENSE
 *   This software is dual-licensed to the public domain and under the following
 *   license: you are granted a perpetual, irrevocable license to copy, modify,
 *   publish, and distribute this file as you see fit.
 *   No warranty implied; use at your own risk.
 *
 * So you can do whatever you want with this code, including copying it
 * (or parts of it) into your own source.
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */

#ifndef DG__FILEHANDLING_H
#define DG__FILEHANDLING_H

#ifdef __cplusplus
extern "C" {
#endif

// this allows you to prepend stuff to function signatures, e.g. "static"
#ifndef DGFH_DEF
  // by default it's empty
  #define DGFH_DEF
#endif // DG_MISC_DEF

enum DGFH_TYPE {
	DGFH_ALL          = -1, // accept all file types (argument for dgfh_opendir())
	DGFH_UNKNOWN      = 0,  // unknown or invalid type, file doesn't exist or other error
	DGFH_REGULAR_FILE = 1,  // just a regular file
	DGFH_DIRECTORY    = 2,  // directory
	DGFH_SYMLINK      = 4,  // symbolic link
	DGFH_SOCKET       = 8,  // Unix Domain socket
	DGFH_FIFO         = 16, // FIFO (named pipe)
	DGFH_CHAR_DEVICE  = 32, // character device
	DGFH_BLOCK_DEVICE = 64, // block device

	// TODO: DGFH_HIDDEN ??
};

enum {
	DGFH_PATH_MAX = 4096 // should be enough for everyone, I hope :-P
};

typedef struct dgfh_dirent
{
	int type; // type of this file (DGFH_REGULAR_FILE or DGFH_DIRECTORY or whatever)
	char name[DGFH_PATH_MAX]; // filename (relative to the directory you passed to dgfh_opendir())
} dgfh_dirent;

typedef struct dgfh_dir dgfh_dir; // opaque type

/* Allows iterating the entries of a directory, kinda like opendir()
 * directory_name: path of the directory to list
 * accepted_types: OR-ed list of entry types you want (DGFH_REGULAR_FILE for normal files,
 *   DGFH_DIRECTORY for directories, DGFH_SOCKET | DGFH_FIFO for unix domain sockets and pipes,
 *   DGFH_ALL if you just want all types of directory entries
 *
 * Returns: dgfh_dir* that you can use with dgfh_next_dir_entry(), NULL on error (like invalid directory)
 * Don't forget to call dgfh_close_dir() when you're done!
 */
DGFH_DEF dgfh_dir* dgfh_opendir(const char* directory_name, int accepted_types); // TODO: filename pattern?

// returns the next directory entry (dgfh_dirent) of dir, NULL if there are no more entries
DGFH_DEF dgfh_dirent* dgfh_next_dir_entry(dgfh_dir* dir);

// closes the given dgfh_dir, call this when you're done calling dgfh_next_dir_entry()
// returns 0 on success TODO: or maybe some enum value or whatever?
DGFH_DEF int dgfh_close_dir(dgfh_dir* dir);

// TODO: functions to delete, rename, copy files (and maybe even whole directories?)

#ifdef _WIN32 // UTF-8 wrappers for standard-functions

  // fopen()-wrapper that uses UTF-8 for filename (instead of whatever 8bit charset your windows installation uses)
  DGFH_DEF FILE* fopen_utf8(const char* filename, const char* mode);

  // chdir()/_chdir()-wrapper that uses UTF-8 for filename (instead of whatever 8bit charset your windows installation uses)
  DGFH_DEF int chdir_utf8(const char* directory_name);

  // TODO: maybe some windows-only wrappers for common WinAPI functions like CreateFile()

#else // not Windows - should support UTF-8 natively, just alias the normal functions

  // fopen() that accepts UTF-8 argument.. just an alias on this platform
  #define fopen_utf8(filename, mode)  fopen(filename, mode)

  // chdir() that accepts UTF-8 argument.. just an alias on this platform
  #define chdir_utf8(dirname)  chdir(dirname)

#endif // not Windows

#ifdef __cplusplus
} // extern "C"
#endif

#endif // DG__FILEHANDLING_H


// #################### Below: Implementation ############################

#ifdef DG_FILEHANDLING_IMPLEMENTATION

#ifndef _WIN32 // might need to be changed for other non-POSIX OSs, if any
#define _DGFH_POSIX
#endif

// no idea which netbsd version introduced this (linux: glibc 2.4)
#if defined(__linux) || defined(__NetBSD__) \
   || (defined(__FreeBSD_version) && __FreeBSD_version >= 800000) || defined(__OpenBSD__) // since OpenBSD 5.0 - how to check?
   // note that OSX does not support this (up to version 10.13 at least)
#define _DGFH_HAVE_FSTATAT
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef _DGFH_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
 #include <dirent.h>
#endif // _DGFH_POSIX

#ifdef _WIN32
// TODO: lean and mean or sth like that?
#include <windows.h>
#endif // _WIN32

#define DGFH_ASSERT(cond, msg) \
	assert((cond) && (msg))

struct dgfh_dir
{
	dgfh_dirent dir_entry; // this is returned by dgfh_next_dir_entry()
	int accepted_types;

#ifdef _WIN32
	WIN32_FIND_DATAW find_data;
	HANDLE handle;
	BOOL have_more_data;
#endif

#ifdef _DGFH_POSIX
	DIR* directory;

#ifdef _DGFH_HAVE_FSTATAT
	int dir_fd;
#endif

	char name[DGFH_PATH_MAX];
	size_t name_len;
#endif
};

#ifdef _DGFH_POSIX

static size_t _dgfh_strlcpy(char* dst, const char* src, size_t dstsize)
{
	DGFH_ASSERT(src && dst, "Don't call strlcpy with NULL arguments!");
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

static size_t _dgfh_strlcat(char* dst, const char* src, size_t dstsize)
{
	DGFH_ASSERT(src && dst, "Don't call strlcat with NULL arguments!");

	size_t dstlen = strnlen(dst, dstsize);
	size_t srclen = strlen(src);

	DGFH_ASSERT(dstlen != dstsize, "dst must contain null-terminated data with strlen < dstsize!");

	// TODO: dst[dstsize-1] = '\0' to ensure null-termination and make wrong dstsize more obvious?

	if(dstsize > 1 && dstlen < dstsize-1)
	{
		size_t numchars = dstsize-dstlen-1;

		if(srclen < numchars)  numchars = srclen;

		memcpy(dst+dstlen, src, numchars);
		dst[dstlen+numchars] = '\0';
	}

	return dstlen + srclen;
}

DGFH_DEF dgfh_dir* dgfh_opendir(const char* directory_name, int accepted_types)
{
	dgfh_dir* ret = NULL;
	DIR* dir = NULL;

	size_t dirname_len = strlen(directory_name);
	if(dirname_len > DGFH_PATH_MAX-2) // -2 leaves space to add a '/' and terminating '\0'
	{
		// TODO: assert(0); ?? log error or sth?
		return NULL;
	}
	if(accepted_types == 0)  accepted_types = DGFH_ALL;

	dir = opendir(directory_name);
	if(dir == NULL)  return NULL;

	ret = (dgfh_dir*)calloc(1, sizeof(dgfh_dir));
	if(ret == NULL)
	{
		closedir(dir);
		return NULL;
	}

	ret->directory = dir;
	memcpy(ret->name, directory_name, dirname_len+1);
	ret->name_len = dirname_len;
	ret->accepted_types = accepted_types;

#ifdef _DGFH_HAVE_FSTATAT
	ret->dir_fd = dirfd(dir);
#endif

	return ret;
}

DGFH_DEF dgfh_dirent* dgfh_next_dir_entry(dgfh_dir* dir)
{
	// TODO: assert(dir);
	if(dir != NULL)
	{
		dgfh_dirent* ret = &dir->dir_entry;
		DIR* directory = dir->directory;
		int accepted_types = dir->accepted_types;

		for(struct dirent* ent = readdir(directory); ent != NULL; ent = readdir(directory))
		{
			const char* name = ent->d_name;
			int type = ent->d_type;
			int our_type = DGFH_UNKNOWN; //  the entries type translated to DGFH_*  type

			// skip "." and ".."
			if(name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')))  continue;

			if(type != DT_UNKNOWN)
			{
				switch(type)
				{
					case DT_REG:  our_type = DGFH_REGULAR_FILE; break;
					case DT_DIR:  our_type = DGFH_DIRECTORY; break;
					case DT_LNK:  our_type = DGFH_SYMLINK; break;
					case DT_SOCK: our_type = DGFH_SOCKET; break;
					case DT_FIFO: our_type = DGFH_FIFO; break;
					case DT_CHR:  our_type = DGFH_CHAR_DEVICE; break;
					case DT_BLK:  our_type = DGFH_BLOCK_DEVICE; break;
					default:      our_type = DGFH_UNKNOWN;
				}
			}
			else // it's DT_UNKNOWN => gotta stat()
			{
				struct stat stat_buf = {0};
#ifdef _DGFH_HAVE_FSTATAT
				// try fstatat() first
				if(dir->dir_fd <= 0 || fstatat(dir->dir_fd, name, &stat_buf, 0) != 0)
#endif
				{
					// fall back to normal stat() which implies concatenating the paths first
					char full_path[DGFH_PATH_MAX];
					int dir_len = dir->name_len;
					memcpy(full_path, dir->name, dir_len);
					full_path[dir_len] = '/';
					full_path[dir_len+1] = '\0';
					if(_dgfh_strlcat(full_path, name, DGFH_PATH_MAX) > DGFH_PATH_MAX
					   || stat(full_path, &stat_buf) != 0)
					{
						stat_buf.st_mode = 0;
					}
				}

				switch(stat_buf.st_mode & S_IFMT)
				{
					case 0: our_type = DGFH_UNKNOWN; break; // some error happened in stat(), try next file
					case S_IFREG:  our_type = DGFH_REGULAR_FILE; break;
					case S_IFDIR:  our_type = DGFH_DIRECTORY; break;
					case S_IFLNK:  our_type = DGFH_SYMLINK; break;
					case S_IFSOCK: our_type = DGFH_SOCKET; break;
					case S_IFIFO:  our_type = DGFH_FIFO; break;
					case S_IFCHR:  our_type = DGFH_CHAR_DEVICE; break;
					case S_IFBLK:  our_type = DGFH_BLOCK_DEVICE; break;
					default:       our_type = DGFH_UNKNOWN;
				}
			}

			// TODO: if(our_type != 0 && ent->d_name[0] == '.')  our_type |= DGFH_HIDDEN; ??

			if(our_type & accepted_types)
			{
				_dgfh_strlcpy(ret->name, name, DGFH_PATH_MAX);
				ret->type = our_type;
				return ret;
			}
		}
	}
	return NULL;
}


DGFH_DEF int dgfh_close_dir(dgfh_dir* dir)
{
	if(dir != NULL)
	{
		if(dir->directory != NULL)
		{
			closedir(dir->directory);
		}
		free(dir);
		return 1;
	}
	return 0;
}


#elif defined(_WIN32)

DGFH_DEF dgfh_dir* dgfh_opendir(const char* directory_name, int accepted_types)
{
	dgfh_dir* ret = NULL;
	WIN32_FIND_DATAW find_data = {0};
	HANDLE h = INVALID_HANDLE_VALUE;
	WCHAR nameW[DGFH_PATH_MAX] = {0};
	// convert directory_name to WCHAR
	int len = MultiByteToWideChar(CP_UTF8, 0, directory_name, -1, nameW, DGFH_PATH_MAX);
	WCHAR* append_str = L"/*.*";
	int append_str_len = 4;
	if(len <= 0)  return NULL;

	if(nameW[len-1] == L'/' || nameW[len-1] == L'\\')
	{
		append_str = L"*.*";
		append_str_len = 3;
	}

	if(wcsncat_s(nameW, DGFH_PATH_MAX, append_str, append_str_len) != 0)
	{
		return NULL;
	}

	h = FindFirstFileW(nameW, &find_data);
	if(h == INVALID_HANDLE_VALUE)  return NULL;

	ret = (dgfh_dir*)calloc(1, sizeof(dgfh_dir));
	if(ret == NULL)
	{
		FindClose(h);
		return NULL;
	}

	ret->handle = h;
	ret->find_data = find_data;
	ret->accepted_types = accepted_types;
	ret->have_more_data = TRUE;
	return ret;
}


DGFH_DEF dgfh_dirent* dgfh_next_dir_entry(dgfh_dir* dir)
{
	if(dir != NULL && dir->have_more_data)
	{
		int accepted_types = dir->accepted_types;
		dgfh_dirent* dirent = &dir->dir_entry;
		WIN32_FIND_DATAW* find_data = &dir->find_data;
		HANDLE handle = dir->handle;
		for(BOOL have_more_data = dir->have_more_data; have_more_data;
		    have_more_data = FindNextFileW(handle, find_data))
		{
			DWORD file_attr = find_data->dwFileAttributes;
			const WCHAR* name = find_data->cFileName;

			int our_type = DGFH_REGULAR_FILE;

			// skip "." and ".."
			if(name[0] == L'.' && (name[1] == 0 || (name[1] == L'.' && name[2] == 0)))  continue;

			if(file_attr & FILE_ATTRIBUTE_DIRECTORY)    our_type = DGFH_DIRECTORY;
			else if(file_attr & FILE_ATTRIBUTE_DEVICE)  our_type = DGFH_BLOCK_DEVICE; // TODO: or DGFH_CHAR_DEVICE ??
			// TODO: FILE_ATTRIBUTE_OFFLINE => DGFH_UNKNOWN ? other flags that should be handled that way?

			/* TODO: hidden flag?
			if((file_attr & FILE_ATTRIBUTE_HIDDEN) || (file_attr & FILE_ATTRIBUTE_SYSTEM))  our_type |= DGFH_HIDDEN;
			*/

			if(our_type & accepted_types)
			{
				// convert the WCHAR filename from find_data to UTF-8, save directly in dirent->name
				if(WideCharToMultiByte(CP_UTF8, 0, name, -1, dirent->name, DGFH_PATH_MAX, NULL, NULL) > 0)
				{
					dirent->type = our_type;

					// make sure dir->have_more_data and dir->find_data always contain the *next* search result
					// (because it starts with the one from FindFirstFileW() in dgfh_opendir())
					dir->have_more_data = FindNextFileW(handle, find_data);
					return dirent;
				}
			}
		}

		// we only get here if we found no matching file
		// and FindNextFileW() returned FALSE (=> no more files in directory)
		dir->have_more_data = FALSE;
	}
	return NULL;
}


DGFH_DEF int dgfh_close_dir(dgfh_dir* dir)
{
	if(dir != NULL)
	{
		FindClose(dir->handle);
		free(dir);
		return 1;
	}
	return 0;
}

// some UTF-8 wrappers for standard (and maybe WinAPI) functions

DGFH_DEF FILE* fopen_utf8(const char* filename, const char* mode)
{
	WCHAR nameW[DGFH_PATH_MAX] = { 0 };
	WCHAR modeW[16] = { 0 };
	int len = MultiByteToWideChar(CP_UTF8, 0, filename, -1, nameW, DGFH_PATH_MAX);
	if(len > 0 && MultiByteToWideChar(CP_UTF8, 0, mode, -1, modeW, 16) > 0)
	{
		// this shuts up the complaints about _wfopen() being unsafe..
		// TODO: use return _wfopen(nameW, modeW); on older compilers that don't support _wfopen_s()? (which?)
		FILE* ret = NULL;
		if(_wfopen_s(&ret, nameW, modeW) == 0)  return ret;
	}
	return NULL;
}

DGFH_DEF int chdir_utf8(const char* directory_name)
{
	WCHAR nameW[DGFH_PATH_MAX] = { 0 };
	if(MultiByteToWideChar(CP_UTF8, 0, directory_name, -1, nameW, DGFH_PATH_MAX) > 0)
	{
		return _wchdir(nameW);
	}
	return 1; // TODO: or what?
}

#else
#error "Implement dgfh_opendir(), dgfh_next_dir_entry() and dgfh_close_dir() for your platform"
#endif

#endif // DG_FILEHANDLING_IMPLEMENTATION
