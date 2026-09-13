/*
 * A stringbuffer/stringbuilder for C99.
 *
 * In *one* .c file do
 *   #define DG_STRINGBUFFER_IMPL
 *   #include "DG_stringbuffer.h"
 *
 * In other sourcefiles that need it use #include without the #define
 *
 * Copyright (c) 2026 Daniel Gibson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#if 0 // Example:

// TODO

#endif

#ifndef _DG_STRINGBUFFER_H_
#define _DG_STRINGBUFFER_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#if SIZE_MAX == 0xFFFFFFFFul
  typedef int32_t dg_ssize_t;
#elif SIZE_MAX == 0xFFFFFFFFFFFFFFFFull
  typedef int64_t dg_ssize_t;
#else
  #error "Unsupported architecture"
#endif

#define DG_SSIZE_MAX (SIZE_MAX/2)

typedef struct dg_stringbuf {
	char* s;
	size_t len; // current length of string (WITHOUT terminating \0)
	size_t cap; // current capacity of buffer s points to (with terminating \0)
	bool external_data; // data will not to be freed by dg_sb_free()
	// TODO: char _intbuf[sizeof(size_t)-sizeof(bool)]; except what if both are 4 bytes
} dg_sb;


// returns an empty stringbuffer that's ready to use.
// don't forget to free it eventually with dg_sb_free()!
// use like:
//   dg_sb myStringBuffer = dg_sb_init();
//   dg_sb_adds(&myStringBuffer, "Hello, World number ");
//   dg_sb_add_int(&myStringBuffer, 42);
//   dg_sb_addf(&myStringBuffer, "\nBut also, %s bye!\n", byeIsGood ? "good" : "bad");
//   printf("myStringBuffer contains: '%s'\n", myStringBuffer.s);
//   dg_sb_free(&myStringBuffer);
static inline
dg_sb dg_sb_init(void)
{
	// setting external_data to true because "" can't be free'd
	// (setting "" instead of NULL so printing ret.s works as expected)
	dg_sb ret = { (char*)"", 0, 0, true };
	return ret;
}

// NOTE: dg_sb myStringBuffer = {0}; and then adding to it also works,
//       only difference to dg_sb myStringBuffer = dg_sb_init(); is that
//       myStringBuffer.s will be NULL instead of empty string until you add anything

// create stringbuffer that reserves an initial capacity,
// but still has logical length 0
extern
dg_sb dg_sb_init_res(size_t init_capacity);

// create stringbuffer that uses a provided buffer
// will still allocate heap memory if the provided buffer is too small,
// so don't forget to call dg_sb_free() eventually!
static inline
dg_sb dg_sb_init_external(char* buf, size_t buf_cap)
{
	dg_sb ret = { buf, 0, buf_cap, true };
	return ret;
}

// frees the data held by a stringbuffer (in sb->s) and resets sb so it's empty
// like right after dg_sb_init() (which means it can be added to again).
//
// ! This is for stringbuffers that live on the stack, for stringbuffers !
// ! created with dg_sb_new_onheap*() use dg_sb_delete_onheap() instead  !
extern
void dg_sb_free(dg_sb* sb);


// create stringbuffer that is allocated on the heap, unlike dg_sb_init*() where
// the stringbuffer itself is on the stack and only its data might be on the heap.
// if init_cap > 0, that many bytes will be allocated in addition to the size
// of the dg_sb struct to hold a buffer that will be used like an "external" one,
// until it's too small, then fresh memory will be allocated on the heap
// as usual and this buffer behind the dg_sb struct will be unused
//
// ! Must be free'd with dg_sb_delete_onheap() !
// returns NULL if out-of-memory (OOM)
extern
dg_sb* dg_sb_new_onheap_incl_data(size_t init_cap);

// create stringbuffer that is allocated on the heap, unlike dg_sb_init*() where
// the stringbuffer itself is on the stack and only its data might be on the heap.
//
// ! Must be free'd with dg_sb_delete_onheap() !
// returns NULL if out-of-memory (OOM)
static inline
dg_sb* dg_sb_new_onheap(void)
{
	return dg_sb_new_onheap_incl_data(0);
}

// create stringbuffer that is allocated on the heap, unlike dg_sb_init*() where
// the stringbuffer itself is on the stack and only its data might be on the heap.
// like in dg_sb_init_external(), the provided buf will be used for the string,
// until it's too small, then new memory will be allocated on the heap
//
// ! Must be free'd with dg_sb_delete_onheap() !
// returns NULL if out-of-memory (OOM)
static inline
dg_sb* dg_sb_new_onheap_external(char* buf, size_t buf_cap)
{
	dg_sb* ret = dg_sb_new_onheap_incl_data(0);
	ret->cap = buf_cap;
	ret->s = buf;
	return ret;
}

// frees a dg_sb* ! that has been allocated with dg_sb_new_onheap*() !
// sb is expected to be a pointer to dg_sb and will be set to NULL
#define dg_sb_onheap_delete(sb) \
	_dg_sb_onheap_delete_impl(&(sb))


// clears string but doesn't free the buffer (sb->cap remains unchanged)
// like dg_sb_truncate(sb, 0);
static inline
void dg_sb_clear(dg_sb* sb)
{
	if (sb != NULL && sb->len > 0) {
		sb->s[0] = '\0';
		sb->len = 0;
	}
}

// truncates string to length new_len (which must be <= sb->len)
// returns new length, does NOT change capacity
// returns 0 on error
static inline
size_t dg_sb_truncate(dg_sb* sb, size_t new_len)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	if (new_len > sb->len) {
		assert(0 && "truncate is supposed to reduce the size, not grow");
	} else if(new_len < sb->len) {
		sb->s[new_len] = '\0';
		sb->len = new_len;
	}
	return sb->len;
}

// delete n characters starting at pos
// returns logical length after operation (sb->len)
//   that should be old sb->len - n, except if you
//   passed a too big n, then this just truncats sb to pos bytes (and returns pos)
// returns 0 on error (like out-of-memory)
extern
size_t dg_sb_delete(dg_sb* sb, size_t pos, size_t n);

// make sure sb->s can hold at least capacity chars.
// returns actually available capacity after operation, which might be > capacity
// returns 0 on error (out-of-memory or if sb was NULL)
extern
size_t dg_sb_reserve(dg_sb* sb, size_t capacity);

// make sure sb->s can hold at least add_chars additional chars
// (i.e. in addition to the sb->len logical chars already held)
// returns actually available capacity after operation, which might be > sb->len + add_chars
// returns 0 on error (out-of-memory or if sb was NULL)
static inline
size_t dg_sb_reserve_add(dg_sb* sb, size_t add_chars)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	if (SIZE_MAX - sb->len <= add_chars) {
		// TODO: overflow
		return 0;
	}
	size_t res = sb->len + add_chars + 1; // +1 for terminating \0
	return (res <= sb->cap) ? sb->cap : dg_sb_reserve(sb, res);
}


// modify length of sb, if it's bigger than the current length, fill the resulting
// added chars with fill_char
// returns logical length after operation (sb->len)
// returns 0 on error (like out-of-memory)
static inline
size_t dg_sb_set_length(dg_sb* sb, size_t new_len, char fill_char)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	if (new_len == sb->len) {
		return new_len; // nothing to do
	} else if (new_len > sb->len) {
		if (new_len >= sb->cap && dg_sb_reserve(sb, new_len) == 0) {
			return 0;
		}
		memset(sb->s + sb->len, fill_char, new_len - sb->len);
	}

	sb->s[new_len] = '\0';
	sb->len = new_len;
	return new_len;
}


// add len chars of data to sb
// returns logical length of string after adding data (without terminating \0)
// returns 0 on error, like out-of-memory (or if sb->len and len both were 0)
extern
size_t dg_sb_add(dg_sb* sb, const char* data, size_t len);

// add single char
// returns logical length of string after adding c (without terminating \0)
// returns 0 on error, like out-of-memory
static inline
size_t dg_sb_addc(dg_sb* sb, const char c)
{
	if (sb->cap > sb->len + 1) {
		// fast path
		sb->s[sb->len] = c;
		sb->len++;
		sb->s[sb->len] = '\0';
		return sb->len;
	}
	return dg_sb_add(sb, &c, 1);
}


// add strlen(c_str) bytes of data to sb
// returns logical length of string after adding c_str (without terminating \0)
// returns 0 on error, like out-of-memory (or if sb->len and len both were 0)
static inline
size_t dg_sb_adds(dg_sb* sb, const char* c_str)
{
	size_t len = (c_str != NULL) ? strlen(c_str) : 0;
	return dg_sb_add(sb, c_str, len);
}

// append other string buffer to sb
// returns logical length of string after adding it (without terminating \0)
// returns 0 on error, like out-of-memory (or if sb->len and other->len both were 0)
static inline
size_t dg_sb_addsb(dg_sb* sb, const dg_sb* other)
{
	return other ? dg_sb_add(sb, other->s, other->len) : sb->len;
}

// append printf-formatted string to sb (when using a va_list)
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory (or if sb->len was 0 and formatted string empty)
extern
size_t dg_sb_addvf(dg_sb* sb, const char* fmt, va_list ap);

// append printf-formatted string to sb
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory (or if sb->len was 0 and formatted string empty)
static inline
size_t dg_sb_addf(dg_sb* sb, const char* fmt, ...) // TODO: printf-annotation
{
	va_list ap;
	va_start(ap, fmt);
	size_t ret = dg_sb_addvf(sb, fmt, ap);
	va_end(ap);
	return ret;
}

static inline
// TODO: separator string to put between elements?
size_t dg_sb_add_multiple(dg_sb* sb, const char* strings[], size_t num_strings)
{
	size_t addlen = 0;
	for (size_t i=0; i < num_strings; ++i) {
		size_t len = strlen(strings[i]);
		if (SIZE_MAX - addlen <= len) {
			// TODO: overflow
			return 0;
		}
		addlen += len;
	}
	// reserve enough space for all strings that should be added
	if (dg_sb_reserve_add(sb, addlen) == 0) {
		return 0;
	}

	for (size_t i=0; i < num_strings; ++i) {
		// TODO: would be nicer to reuse the strlen() from above, but that would
		//       require an additional array...
		dg_sb_add(sb, strings[i], strlen(strings[i]));
	}

	return sb->len;
}

// FIXME: the following most probably needs different syntax, if it works at all
#define dg_sb_add_all(sb, ...) \
	dg_sb_add_multiple(sb, { __VA_ARGS__ }, sizeof( { __VA_ARGS__ } )/sizeof(const char*) )

// TODO: sth for string literals (=> length known at compiletime), if those can be identified?
//       might need compiler-specific hacks...

// add an integer, converted to string
// - x is the integer to add
// - base is the number base to be used for formatting, e.g. 2 for binary, 16 for hex or 10 for (standard) decimal
// - prefix could be "0x" when using base=16, "0b" for base=2 or NULL for no prefix
//   (or whatever you want..). if x < 0, '-' will be prepended before the prefix
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory
extern
size_t dg_sb_add_int_ext(dg_sb* sb, int64_t x, int base, const char* prefix);

// add an unsigned integer, converted to string
// - ux is the unsigned integer to add
// - base is the number base to be used for formatting, e.g. 2 for binary, 16 for hex or 10 for (standard) decimal
// - prefix could be "0x" when using base=16, "0b" for base=2 or NULL for no prefix
//   (or whatever you want..)
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory
extern
size_t dg_sb_add_uint_ext(dg_sb* sb, uint64_t ux, int base, const char* prefix);

// add an integer, converted to string (as decimal)
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory
static inline
size_t dg_sb_add_int(dg_sb* sb, int64_t i)
{
	return dg_sb_add_int_ext(sb, i, 10, NULL);
}

// add an unsigned integer, converted to string (as decimal)
// returns logical length of string after the operation (without terminating \0)
// returns 0 on error, like out-of-memory
static inline
size_t dg_sb_add_uint(dg_sb* sb, uint64_t ui)
{
	return dg_sb_add_uint_ext(sb, ui, 10, NULL);
}

// Note: I'm not gonna implement a proper float formatter, just use dg_sb_addf(sb, "%f", 1.23);



// Create a duplicate of other_sb (same length, same data up to length, possibly different capacity)
// Just like an dg_sb created with dg_sb_init*(), it must be free'd eventually with dg_sb_free(&my_sb); !
static inline
dg_sb dg_sb_dup(const dg_sb* other_sb)
{
	dg_sb ret = {0};
	dg_sb_add(&ret, other_sb->s, other_sb->len);
	return ret;
}

// implementation details
extern
void _dg_sb_onheap_delete_impl(dg_sb** sbp);

// TODO: insert, delete, replace

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _DG_STRINGBUFFER_H_

// =========================
//  Below: Implementation

#ifdef DG_STRINGBUFFER_IMPL

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

dg_sb dg_sb_init_res(size_t init_capacity)
{
	dg_sb ret = {0};
	ret.s = malloc(init_capacity);
	if (ret.s == NULL) {
		// TODO: OOM
		return ret;
	}
	ret.s[0] = '\0';
	ret.cap = init_capacity;
	return ret;
}

dg_sb* dg_sb_new_onheap_incl_data(size_t init_cap)
{
	if (SIZE_MAX-sizeof(dg_sb) <= init_cap) {
		assert( 0 && "init_cap way too huge" );
		return NULL;
	}
	char* data = (char*)malloc(sizeof(dg_sb) + init_cap);
	if (data == NULL) {
		// TODO: OOM
		return NULL;
	}
	
	dg_sb* ret = (dg_sb*)data;
	if (init_cap > 0) {
		ret->s = data + sizeof(dg_sb);
		ret->s[0] = '\0';
	} else {
		ret->s = "";
	}
	ret->cap = init_cap;
	ret->external_data = true;
	ret->len = 0;
	return ret;
}

void _dg_sb_delete_onheap_impl(dg_sb** sbp)
{
	if (sbp != NULL && *sbp != NULL) {
		dg_sb* sb = *sbp;
		if (!sb->external_data) {
			free(sb->s);
		}
		free(sb);
		*sbp = NULL;
	}
}

void dg_sb_free(dg_sb* sb)
{
	if (sb != NULL) {
		if (sb->external_data) {
			if (sb->cap > 0) {
				sb->s[0] = '\0';
			}
		} else {
			free(sb->s);
			sb->s = "";
			sb->cap = 0;
		}
		sb->len = 0;
	}
}

size_t dg_sb_reserve(dg_sb* sb, size_t capacity)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	if (capacity <= sb->cap) {
		return sb->cap;
	}
	if (capacity < 8) {
		// don't allocate just 3 bytes or whatever
		capacity = 8;
	} else if (capacity < SIZE_MAX/2) {
		// according to the internet, either 1.5 or 1.618.. (golden ratio) are optimal
		// growth rates for dynamic arrays, let's use 1.5 (3/2) because it's simple
		// https://en.wikipedia.org/wiki/Dynamic_array#Growth_factor
		// https://github.com/facebook/folly/blob/master/folly/docs/FBVector.md
		// https://stackoverflow.com/questions/1100311/what-is-the-ideal-growth-rate-for-a-dynamically-allocated-array
		size_t pot_cap = (sb->cap / 2) * 3;
		if (pot_cap > capacity) {
			capacity = pot_cap;
		}
	}
	char* b = NULL;
	if (sb->external_data) {
		b = malloc(capacity);
		if (b == NULL) {
			// TODO: OOM
			return 0;
		}
		if (sb->len > 0) {
			memcpy(b, sb->s, sb->len);
		}
		sb->external_data = false;
		b[sb->len] = '\0';
	} else {
		// Note: this implicitly handles the sb->s = NULL case from dg_sb sb = {0};
		//       because realloc() behaves like malloc() if first arg is NULL
		b = realloc(sb->s, capacity);
		if (b == NULL) {
			// TODO: OOM
			return 0;
		}
	}
	sb->s = b;
	sb->cap = capacity;
	return capacity;
}

size_t dg_sb_add(dg_sb* sb, const char* data, size_t len)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	if (len == 0) {
		if (sb->s == NULL) {
			// make sure that after adding to a stringbuffer it points to a valid
			// null-terminated string, even if len == 0
			assert(sb->len == 0 && sb->cap == 0 && "dg_sb's `s` member may only be NULL if its len and cap are NULL!");
			sb->s = (char*)"";
			sb->external_data = true;
			sb->len = sb->cap = 0;
		}
		return sb->len;
	}
	size_t ret = 0;
	if (MAX_SIZE - sb->len > len) {
		ret = sb->len + len;
		if (ret >= sb->cap && dg_sb_reserve(sb, ret+1) == 0) {
			// TODO: OOM
			return 0;
		}
		memcpy(sb->s + sb->len, data, len);
		sb->s[ret] = '\0';
		sb->len = ret;
	} else {
		// TODO: overflow
		return 0;
	}
	return ret;
}

size_t dg_sb_addvf(dg_sb* sb, const char* fmt, va_list ap);
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}
	char buf[16384]; // TODO: what is a good size that easily fits on relevant stacks?

	size_t ret = 0;
	va_list ap2;
	va_copy(ap2, ap);

	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	if (len < 0) {
		// TODO: wtf, shouldn't even happen
		assert( 0 && "your vsnprintf() returned a negative value, that shouldn't happen" );
		va_end(ap2);
		return 0;
	}
	if (len < sizeof(buf)) {
		// buf was big enough to hold the whole formatted string, just append it to sb
		ret = dg_sb_add(sb, buf, len);
	} else {
		// didn't fit into on-stack buffer - but now we know how long the generated string would be
		// => reserve required space in target string and print into it directly
		if (dg_sb_reserve_add(sb, len) > 0) {
			vsnprintf(sb->s + sb->len, sb->cap - sb->len, fmt, ap2);
			sb->len += len;
			ret = sb->len;
		}
		// else: OOM or overflow, ret can remain 0, if we abort in those cases,
		//       dg_sb_reserve() will have handled that
	}

	va_end(ap2);
	return ret;
}

size_t dg_sb_delete(dg_sb* sb, size_t pos, size_t n)
{
	if (sb == NULL) {
		assert( 0 && "Don't pass sb = NULL!" );
		return 0;
	}

	if (pos > sb->len) {
		assert( 0 && "Don't call delete with a pos that's behind the end" ); // TODO: or just silently ignore?
		return sb->len;
	}
	if (n == 0) {
		return sb->len;
	}

	size_t delend;
	if (SIZE_MAX - n < pos || (delend = pos+n) >= sb->len ) {
		// everything after pos is to be deleted
		sb->s[pos] = '\0';
		sb->len = pos;
		return sb->len;
	}
	
	// at this point it's certain that there are chars between pos+n and sb->len
	// that must be moved to pos
	
	size_t num_move = sb->len - delend;
	memmove(sb->s + pos, sb->s + delend, num_move);
	sb->len = pos + num_move; // TODO: or sb->len -= n;
	sb->s[sb->len] = '\0';
	
	return sb->len;
}


// string formatting stuff for dg_sb_add_(u)int(_ext)

static const char _digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// the most common case (base 10) gets its own function, that usually
// allows the compiler
// x == 0 might not be handled, FormatUInt() already did!
static size_t FormatUIntDec(uint64_t x, dg_sb* sb)
{
	char buf[32]; // big enough to hold any uint64_t value as decimal string
	const uint32_t bufLen = sizeof(buf);
	uint32_t i = bufLen-1;
	// Note that we don't need \0-termination here
	do {
		buf[i--] = '0' + x % 10;
		x /= 10;
	} while(x > 0);
	++i; // back to last index written

	return dg_sb_add(sb, &buf[i], bufLen-i);
}

// power of two bases - can do cheaper math ops here than % and /
// baseLog2 is 1 for base 2 (binary number), 2 for base 4, 3 for base 8 (octal number)
// 4 for base 16 (hex number), 5 for base 32; (x == 0 might not be handled, FormatUInt() already did!)
static size_t FormatUIntPOT(uint64_t x, int baseLog2, dg_sb* sb)
{
	char buf[80]; // big enough even for MAX_UINT64 in binary
	const uint32_t bufLen = sizeof(buf);

	// x % 8 == x & 7 == x & ((1<<3)-1) (and 3 is baseLog2 for 8)
	// equivalent for other power of two bases
	const uint64_t baseModMask = ((uint64_t)1 << baseLog2) - 1;

	uint32_t i = bufLen-1;
	// Note that we don't need \0-termination here
	do {
		buf[i--] = _digits[x & baseModMask];
		x = x >> baseLog2; // equivalent to x /= base; because x / 8 == x >> 3
	} while(x > 0);
	++i; // back to last index written

	return dg_sb_add(sb, &buf[i], bufLen-i);
}

// formating numbers with generic bases between 3 and 36
// x == 0 might not be handled, FormatUInt() already did!
static size_t FormatUIntGeneric(uint64_t x, int base, dg_sb* sb)
{
	char buf[64]; // big enough even for MAX_UINT64 in base3
	const uint32_t bufLen = sizeof(buf);

	uint32_t i = bufLen-1;
	// Note that we don't need \0-termination here
	do {
		buf[i--] = _digits[x % base];
		x /= base;
	} while(x > 0);
	++i; // back to last index written

	return dg_sb_add(sb, &buf[i], bufLen-i);
}

// formats the given unsigned int with the given base as a string
// and appends that to outStringAppendTo
static size_t FormatUInt(uint64_t x, int base, dg_sb* sb)
{
	// at this point we're certain base is between 2 and 36, dg_sb_add_(u)int_ext check that!

	if(x < 2) { // get these special cases (0 and 1, that are identical for all bases) out of the way
		return dg_sb_addc(sb, '0'+x);
	}

	switch(base) {
		// sorted in order of usefulness
		case 10: return FormatUIntDec(x, sb); break;
		case 16: return FormatUIntPOT(x, 4, sb); break;
		case  2: return FormatUIntPOT(x, 1, sb); break;
		case  8: return FormatUIntPOT(x, 3, sb); break;
		case  4: return FormatUIntPOT(x, 2, sb); break;
		case 32: return FormatUIntPOT(x, 5, sb); break;
		default: return FormatUIntGeneric(x, base, sb); break;
	}
}


size_t dg_sb_add_int_ext(dg_sb* sb, int64_t x, int base, const char* prefix)
{
	uint64_t ux = x;

	if (base < 2 || base > 36) {
		assert(0 && "dg_sb_add_int_ext() only works with bases between 2 and (incl.) 36!");
		return 0;
	}

	if (x < 0) {
		// convert x to positive unsigned number ux
		// just ux = -x doesn't work because -x would still be signed and -INT64_MIN doesn't fit into signed int64
		// according to http://graphics.stanford.edu/~seander/bithacks.html#IntegerAbs the following should work:
		ux = -ux; // == -(uint64_t)x; as ux = x above

		// also, prepend '-'
		if (dg_sb_addc(sb, '-') == 0) {
			return 0;
		}
	}

	size_t plen = prefix ? strlen(prefix) : 0;
	if (plen > 0 && dg_sb_add(sb, prefix, plen) == 0) {
		return 0;
	}

	return FormatUInt(ux, base, sb);
}


size_t dg_sb_add_uint_ext(dg_sb* sb, uint64_t ux, int base, const char* prefix)
{
	if (base < 2 || base > 36) {
		assert(0 && "dg_sb_add_uint_ext() only works with bases between 2 and (incl.) 36!");
		return 0;
	}

	size_t plen = prefix ? strlen(prefix) : 0;
	if (plen > 0 && dg_sb_add(sb, prefix, plen) == 0) {
		return 0;
	}

	return FormatUInt(ux, base, sb);
}

#ifdef __cplusplus
} //extern "C"
#endif

#endif // _DG_STRINGBUFFER_IMPL_
