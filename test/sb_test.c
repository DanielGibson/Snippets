#define DG_STRINGBUFFER_IMPL
#define DG_SB_VSNPRINTF_BUFSIZE 8 // ridiculously small so we can test fallback
#include "../DG_stringbuffer.h"

int main() {

	DG_SB_AUTOFREE sb = dg_sb_init();

	assert(sb.s != NULL && sb.s[0] == '\0');

	dg_sb_adds(&sb, "asdf");

	dg_sb_add_int(&sb, 1234);

	dg_sb_addf(&sb, " What %s Ever ", "fucking");

	dg_sb_add_uint_ext(&sb, 0xdeadbeef, 16, "0x");

	printf("sb: '%s'\n", sb.s);
	assert(strcmp(sb.s, "asdf1234 What fucking Ever 0xDEADBEEF") == 0);

	dg_sb sb2 = dg_sb_dup(&sb);

	dg_sb_delete(&sb2, 4, 2);
	assert(strcmp(sb2.s, "asdf34 What fucking Ever 0xDEADBEEF") == 0);

	dg_sb_insertf(&sb2, 11, "%.2f", 1.234f); // this uses vsnprintf() with the stackbuffer
	assert(strcmp(sb2.s, "asdf34 What1.23 fucking Ever 0xDEADBEEF") == 0);

	char extbuf[128];
	// this makes sure that the unused parts of extbuf aren't '\0'
	// and thus helps testing that inserting adds a terminating 0 at the end of the string
	memset(extbuf, 'A', sizeof(extbuf));
	dg_sb sb3 = dg_sb_init_external(extbuf, sizeof(extbuf));
	assert(strncmp(sb3.s, "AAAA", 4) == 0);
	assert(strncmp(sb3.s, "AAAA", 5) != 0);

	dg_sb_addsb(&sb3, &sb2);

	// this uses vsnprintf() with printing directly into the target because the stackbuffer
	// is too small (set to 7 for testing purposes with DG_SB_VSNPRINTF_BUFSIZE)
	dg_sb_insertf(&sb2, 18, "mbling and %s fra", "also, surprisingly");
	assert(strcmp(sb2.s, "asdf34 What1.23 fumbling and also, surprisingly fracking Ever 0xDEADBEEF") == 0);

	dg_sb_insertf(&sb3, 18, "mbling and %s fra", "also, surprisingly");
	assert(strcmp(sb3.s, "asdf34 What1.23 fumbling and also, surprisingly fracking Ever 0xDEADBEEF") == 0);
	dg_sb_inserts(&sb3, 6, "5678");
	assert(strcmp(sb3.s, "asdf345678 What1.23 fumbling and also, surprisingly fracking Ever 0xDEADBEEF") == 0);
	assert(sb3.s == extbuf); // so far there was no reason to allocate fresh memory in sb3
	dg_sb_set_length(&sb3, 127, 'x');

	assert(sb3.s == extbuf && sb3.external_data == true && sb3.len == 127 && strlen(sb3.s) == 127); // 127 chars (+terminating \0) fit into extbuf ..
	assert(strcmp(sb3.s, "asdf345678 What1.23 fumbling and also, surprisingly fracking Ever 0xDEADBEEFxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx") == 0);
	dg_sb_addc(&sb3, 'y');
	assert(sb3.s != extbuf && sb3.external_data == false); // .. but one more doesn't so it should now be using a heap-allocated buffer
	assert(strcmp(sb3.s, "asdf345678 What1.23 fumbling and also, surprisingly fracking Ever 0xDEADBEEFxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxy") == 0);

	printf("sb2: '%s'\n", sb2.s);
	dg_sb_free(&sb2);

	dg_sb_free(&sb3);

	// freeing again shouldn't do anything, not even crash
	dg_sb_free(&sb3);
	dg_sb_free(&sb);

	return 0;
}
