/*
 * Tests for DG_memrmem()
 * (C) 2016 Daniel Gibson
 *
 * License:
 *  This software is in the public domain. Where that dedication is not
 *  recognized, you are granted a perpetual, irrevocable license to copy
 *  and modify this file however you want.
 *  No warranty implied; use at your own risk.
 */

#include <stdarg.h>

#define DG_MISC_IMPLEMENTATION
#define DG_MISC_NO_GNU_SOURCE
#include "../DG_misc.h"

static void fail(const char* fromfunc, int line, const char* failmsgfmt, ...)
{
	printf("!!! %s() line %d:\n\t", fromfunc, line);

	va_list argptr;
	va_start( argptr, failmsgfmt );

	vprintf( failmsgfmt, argptr );

	va_end( argptr );

	printf(" !!!\n");
	//assert(0);
	exit(1);
}

// resultIdx < 0 means "expect NULL"
static void testmemrmem(const char* haystack, size_t haystackLen, const char* needle, size_t needleLen,
                        int resultIdx, int line, const char* func)
{
	const char* res = (const char*)DG_memrmem(haystack, haystackLen, needle, needleLen);
	if(resultIdx < 0)
	{
		if(res != NULL)
		{
			fail(func, line, "DG_memrmem( \"%.*s\", %d, \"%.*s\", %d )\n\tdid not return NULL but (haystack + %d)",
			                  (int)haystackLen, haystack, (int)haystackLen,
			                  (int)needleLen, needle, (int)needleLen, (int)(res-haystack));
		}
	}
	else
	{
		if(res == NULL)
		{
			fail(func, line, "DG_memrmem( \"%.*s\", %d, \"%.*s\", %d )\n\tdid not return (haystack + %d) but NULL",
		                  (int)haystackLen, haystack, (int)haystackLen,
	                          (int)needleLen, needle, (int)needleLen, resultIdx);
		}
		else if(haystack + resultIdx != res)
		{
			fail(func, line, "DG_memrmem( \"%.*s\", %d, \"%.*s\", %d )\n\tdid not return (haystack + %d) but (haystack + %d)",
			                  (int)haystackLen, haystack, (int)haystackLen,
			                  (int)needleLen, needle, (int)needleLen, resultIdx, (int)(res-haystack));
		}
	}
}

#define TEST_MEMRMEM(haystack, haystackLen, needle, needleLen, resultIdx) \
	testmemrmem(haystack, haystackLen, needle, needleLen, resultIdx, __LINE__, __FUNCTION__)

#define TEST_MEMRMEM_STR(haystack, needle, resultIdx) \
	testmemrmem(haystack, strlen(haystack), needle, strlen(needle), resultIdx, __LINE__, __FUNCTION__)

void testWithNullterminatedStrings()
{
	//                         111111111122
	//               0123456789012345678901
	const char* s = "#asdfasdfasd2fasdfasd";
	TEST_MEMRMEM_STR(s, s, 0);
	TEST_MEMRMEM_STR(s, "#a", 0);
	TEST_MEMRMEM_STR(s, "#", 0);
	TEST_MEMRMEM_STR(s+1, "#", -1); // no more # if starting at s+1
	TEST_MEMRMEM_STR(s, "asd", 18);
	TEST_MEMRMEM_STR(s, "q", -1);
	TEST_MEMRMEM_STR(s, "2", 12);
	TEST_MEMRMEM_STR(s, "2f", 12);
	TEST_MEMRMEM_STR(s, "2a", -1);
	TEST_MEMRMEM_STR(s, "d2", 11);
	TEST_MEMRMEM_STR(s, "#asdfasdfasd2fasdfasdP", -1); // longer but identical up the strlen(s)
	TEST_MEMRMEM_STR(s, ".........................", -1); // just longer
}

void testWithoutNullTermination()
{
	//                         111111111122
	//               0123456789012345678901
	const char* s = "#asdfasdfasd2fasdfasd";
	size_t slen = strlen(s); // without terminating '\0' at the end!

	TEST_MEMRMEM(s, slen, "\0\0", 2, -1);
	TEST_MEMRMEM(s, slen, "#a", 2, 0);
	TEST_MEMRMEM(s, slen+1, "\0", 1, slen); // if passed strlen(s)+1, the terminating '\0' should be found
	TEST_MEMRMEM(s, slen, "\0", 1, -1); // if just strlen(s), then not
	TEST_MEMRMEM(s, slen+1, "\0", 2, -1); // neither when looking or two '\0's
	TEST_MEMRMEM(s, slen+1, "asd", 3, 18);
	TEST_MEMRMEM(s, slen-1, "asd", 3, 14); // if the last char is "cut off", "asd" won't be there

	// string with \0 in the middle
	//                             111111
	//                  012345 6789012345
	const char s2[] = "\0OI;:B\0AFPOIWQE";
	size_t s2len = sizeof(s2); // incl. terminating '\0' at the end!

	TEST_MEMRMEM(s2, s2len, "\0", 1, s2len-1);
	TEST_MEMRMEM(s2, s2len-1, "\0", 1, 6);
	TEST_MEMRMEM(s2, s2len, "B\0A", 3, 5);
	TEST_MEMRMEM(s2, s2len, "B\0A", 4, -1); // incl term. '\0' of needle
	TEST_MEMRMEM(s2, s2len, "I", 1, 11);
	TEST_MEMRMEM(s2, s2len, "OI", 2, 10);
	TEST_MEMRMEM(s2, 11, "I", 1, 2);
	TEST_MEMRMEM(s2, 12, "I", 1, 11);
	TEST_MEMRMEM(s2, 12, "OI", 2, 10);
	TEST_MEMRMEM(s2, 11, "OI", 2, 1);
	TEST_MEMRMEM(s2, 11, "O", 1, 10);
	TEST_MEMRMEM(s2, 10, "O", 1, 1);
	TEST_MEMRMEM(s2, s2len, "\0O", 2, 0);
	TEST_MEMRMEM(s2, s2len, "WQE\0A", 3, 12); // note that only the first 3 chars of "WQE\0A" are used!
	TEST_MEMRMEM(s2, s2len, "WQE\0A", 4, 12);
	TEST_MEMRMEM(s2, s2len, "WQE\0A", 5, -1);
	TEST_MEMRMEM(s2, s2len, "B\0AB", 3, 5); // only "B\0A" really
	TEST_MEMRMEM(s2, s2len, "B\0AB", 4, -1);
	TEST_MEMRMEM(s2, s2len, s2, s2len, 0);
	TEST_MEMRMEM(s2, s2len-1, s2, s2len, -1);
}

int main()
{
	testWithNullterminatedStrings();

	testWithoutNullTermination();

	printf("Success! All DG_memrmem() tests passed.\n");

	return 0;
}
