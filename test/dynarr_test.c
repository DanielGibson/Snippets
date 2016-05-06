#define DG_DYNARR_IMPLEMENTATION
#define DG_DYNARR_INDEX_CHECK_LEVEL 3
#include "../DG_dynarr.h"

#include <stdio.h>

DA_TYPEDEF(int, MyIntArrType); // you could do this globally in a header

typedef struct {
	int i;
	double d;
} Foo;

DA_TYPEDEF(Foo, FooArray);

static int cmp_Foo(const void* a, const void* b)
{
	const Foo* f1 = (const Foo*)a;
	const Foo* f2 = (const Foo*)b;

	int ret = (f2->i < f1->i) - (f1->i < f2->i);
	if(ret == 0)  ret = (f2->d < f1->d) - (f1->d < f2->d);
	return ret;
}

static int gi = 1;

static int getGi()
{
	gi = !gi;
	return gi;
}

static void testint()
{
	MyIntArrType arr = {0}; // make sure to zero out the struct
	// instead of = {0}; you could also call dg_dynarr_init(arr);

	int bla[10];
	// arr will be able to store 10 elements (in bla) without allocating new memory
	da_init_external(arr,bla,10);
	
	da_push(arr, 42);
	assert(da_count(arr) == 1);
	int* addedElements = da_addn_uninit(arr, 3);
	//int* addedElements = da_addn_zeroed(arr, 3);
	size_t i=0;
	for(i=0; i<3; ++i)
		addedElements[i] = i+5;
	
	assert(da_count(arr) == 4);
/*
	for(i=0; i<da_count(arr); ++i)
		printf("arr[%d] = %d\n", (int)i, arr.p[i]);
*/
	arr.p[2] = 0;

	//da_get(arr, 10);

/*
	for(i=0; i<da_count(arr); ++i)
		printf("arr[%d] = %d\n", (int)i, dg_dynarr_get(arr,i));
*/
	MyIntArrType a2 = {0};
	da_addn(a2, arr.p, da_count(arr)); // copy all elements from arr to a2
	assert(da_count(a2) == 4);

	da_insert(a2, 1, 5);
	assert(da_count(a2) == 5 && a2.p[1] == 5);

	(void)da_addn_zeroed(a2, 3);
	assert(da_count(a2) == 8);

	short s3[3] = {1,2,3};
	// inserting from an array with different type should work correctly,
	// as long as the type can be assigned to the a2's element type
	da_insertn(a2, 1, s3, 3);
	assert(a2.p[1] == 1 && a2.p[2] == 2 && a2.p[3] == 3);
	// same for adding
	da_addn(arr, s3, 3);

	da_clear(arr);
	// make sure da_push() and da_insert() only evaluate the given value
	// once so it's safe to pass things with side-effects
	int x = 0;
	da_push(arr, x++);
	assert(arr.p[0] == 0 && x == 1);
	da_push(arr, ++x);
	assert(arr.p[1] == 2 && x == 2);
	da_insert(arr, 0, x++);
	assert(arr.p[0] == 2 && arr.p[1] == 0 && x == 3);

	gi = 1;
	da_insert(arr, 1, getGi()); // getGi() toggles gi to !gi and returns that
	assert(arr.p[1] == 0 && gi == 0);
	da_push(arr, getGi());
	assert(da_last(arr) == gi && gi == 1);
	
	da_free(arr); // make sure not to leak memory!
	da_free(a2);
}

static int dblEq(double a, double b)
{
	// crappy, but should be good enough to test if the right value is there
	return (a >= b-0.0001 && a <= b+0.0001);
}

static void testfoo()
{
	FooArray fa1;
	da_init(fa1);
	assert(da_count(fa1) == 0 && da_capacity(fa1) == 0 && fa1.p == NULL);

	FooArray fa2 = {0};
	Foo foobuf[5];
	da_init_external(fa2, foobuf, 5);
	assert(da_count(fa2) == 0 && da_capacity(fa2) == 5 && fa2.p == foobuf);

	Foo f = {1, 2.0};
	da_push(fa2, f);
	assert(da_count(fa2) == 1 && da_capacity(fa2) == 5 && fa2.p == foobuf);
	da_push(fa2, f);
	assert(da_count(fa2) == 2 && da_capacity(fa2) == 5 && fa2.p == foobuf);
	(void)da_addn_zeroed(fa2, 3);
	assert(da_count(fa2) == 5 && da_capacity(fa2) == 5 && fa2.p == foobuf);

	for(int i=0; i<3; ++i)  assert(da_get(fa2, i+2).i==0);

	da_push(fa2, f); // now foobuf is full and fa2 had to allocate new memory
	assert(da_count(fa2) == 6 && da_capacity(fa2) == 10 && fa2.p != foobuf);
	da_free(fa2);
	assert(da_count(fa2) == 0 && da_capacity(fa2) == 0 && fa2.p == NULL);

	Foo f2 = {2, 3.0};
	da_add(fa2, f); // da_add is an alias for da_push
	da_insert(fa2, 0, f2);
	assert(da_count(fa2) == 2 && da_capacity(fa2) == 8); // initially 8 elements are allocated
	assert(fa2.p[0].i==2);
	assert(da_get(fa2, 1).i == 1);

	Foo fs[5] = {{3, 4.0}, {4, 5.0}, {5, 6.0}, {6, 7.0}, {7, 8.0}};

	da_addn(fa2, fs, 5);
	assert(da_count(fa2) == 7 && da_capacity(fa2) == 8);
	assert(fa2.p[2].i==3 && fa2.p[6].d==8.0);

	Foo* nfs = da_addn_uninit(fa2, 2);
	assert(da_count(fa2) == 9);
	for(int i=0; i<2; ++i)
	{
		Foo f = {42+i, 1.234};
		nfs[i] = f;
	}
	assert(fa2.p[7].i == 42 && fa2.p[8].i == 43);

	assert(da_count(fa1) == 0);
	da_insertn(fa1, 0, fa2.p, da_count(fa2));
	assert(da_count(fa1) == 9);

	assert(fa1.p[0].i == 2 && fa1.p[1].i == 1 && fa1.p[2].i == 3);

	Foo f3 = {123, 4.56};
	da_insert(fa1, 1, f3);
	assert(da_count(fa1) == 10);
	assert(fa1.p[0].i == 2 && fa1.p[1].i == 123 && fa1.p[2].i == 1 && fa1.p[3].i == 3);

	da_insertn(fa1, 2, fs+1, 2); // {4, 5.0} and {5, 6.0}
	assert(da_count(fa1) == 12);
	assert(fa1.p[0].i == 2 && fa1.p[1].i == 123 && fa1.p[2].i == 4 && fa1.p[3].i == 5 && fa1.p[4].i == 1);

	assert(da_get(fa1, 11).i == 43);
	da_insertn_zeroed(fa1, 11, 12);
	assert(da_count(fa1) == 24);
	
	for(int i=11; i<23; ++i)  assert(fa1.p[i].i == 0 && fa1.p[i].d == 0.0);

	assert(da_get(fa1, 23).i == 43);

	assert(fa1.p[7].i == 5 && dblEq(fa1.p[7].d, 6.0));
	nfs = da_insertn_uninit(fa1, 7, 2);
	assert(da_count(fa1) == 26);
	for(int i=0; i<2; ++i)
	{
		nfs[i].i = i;
		nfs[i].d = i+2.0;
	}
	assert(fa1.p[7].i == 0 && dblEq(fa1.p[7].d, 2.0));
	assert(fa1.p[8].i == 1 && dblEq(fa1.p[8].d, 3.0));
	assert(fa1.p[9].i == 5 && dblEq(fa1.p[9].d, 6.0));
/*
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);
*/
	Foo last = da_last(fa1);
	assert(last.i == 43 && dblEq(last.d, 1.234));
	assert(da_pop(fa1).i == last.i && da_count(fa1) == 25);
	assert(da_pop(fa1).i == 0 && da_count(fa1) == 24);
	da_push(fa1, last);
	assert(da_count(fa1) == 25 && da_last(fa1).i == 43);

	da_delete(fa1, 23);
	assert(da_count(fa1) == 24 && da_last(fa1).i == 43);

	assert(fa1.p[0].i == 2);
	da_delete(fa1, 0);
	assert(fa1.p[0].i == 123 && da_count(fa1) == 23);

	assert(fa1.p[11].i == 42 && fa1.p[21].i == 0 && fa1.p[22].i == 43);
	da_deleten(fa1, 12, 9);
	assert(da_count(fa1) == 14 && fa1.p[11].i == 42 && fa1.p[12].i == 0 && fa1.p[13].i == 43);

	da_deletefast(fa1, 1);
	assert(da_count(fa1) == 13 && fa1.p[1].i == 43 && fa1.p[12].i == 0);

	da_deletenfast(fa1, 3, 3);
	assert(da_count(fa1) == 10 && fa1.p[3].i == 7 && fa1.p[4].i == 42 && fa1.p[5].i == 0 && fa1.p[9].i == 6);

	da_setcount(fa1, 6);
	assert(da_count(fa1) == 6 && da_capacity(fa1) == 36);

	da_setcount(fa1, 8);
	assert(da_count(fa1) == 8 && da_capacity(fa1) == 36);
	// after increasing the count, the old elements should still be there
	// because setcount to <= cap doesn't reallocate
	assert(fa1.p[6].i == 0 && fa1.p[7].i == 1);

	da_shrink_to_fit(fa1);
	assert(da_count(fa1) == 8 && da_capacity(fa1) == 8);

	da_reserve(fa1, 51);
	assert(da_capacity(fa1) == 51 && da_count(fa1) == 8 && !da_oom(fa1));

	assert(da_getptr(fa1, 3) == fa1.p+3 && !da_empty(fa1));
	assert(da_getptr(fa1, 0) == da_begin(fa1) && da_begin(fa1) == fa1.p);
	assert(da_end(fa1) == da_begin(fa1)+da_count(fa1) && da_end(fa1) == da_lastptr(fa1)+1);

	size_t fa2cap = da_capacity(fa2);
	assert(da_count(fa2) == 9 && fa2cap >= da_count(fa2));
	da_clear(fa2);
	assert(da_count(fa2) == 0 && da_capacity(fa2) == fa2cap && da_empty(fa2));
	assert(da_begin(fa2) == fa2.p && da_end(fa2) == fa2.p && da_lastptr(fa2) == NULL);
	da_free(fa2);
	assert(da_count(fa2) == 0 && da_capacity(fa2) == 0 && fa2.p == NULL && da_oom(fa2));
	assert(da_begin(fa2) == NULL && da_end(fa2) == NULL && da_lastptr(fa2) == NULL && da_empty(fa2));
/*
	printf("\n############ cap = %d\n", (int)da_capacity(fa1));
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);
*/
	da_sort(fa1, cmp_Foo);

	for(int i=1, j=0; i<da_count(fa1); ++i, ++j)
	{
		assert(fa1.p[j].i < fa1.p[i].i || fa1.p[j].d < fa1.p[i].d);
	}

	printf("\n############ cap = %d\n", (int)da_capacity(fa1));
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);

	assert(fa1.p[0].i == 0 && fa1.p[1].i == 0 && dblEq(fa1.p[1].d, 2.0) && fa1.p[2].i == 1 && fa1.p[3].i == 5 && da_count(fa1) == 8);
	Foo fx = { 10, 3.4 };
	da_set(fa1, 2, fx);
	assert(fa1.p[0].i == 0 && fa1.p[1].i == 0 && dblEq(fa1.p[1].d, 2.0) && fa1.p[2].i == 10);
	assert(fa1.p[3].i == 5 && fa1.p[4].i == 7 && da_count(fa1) == 8);

	da_setn(fa1, 1, fs+1, 3); // {{4, 5.0}, {5, 6.0}, {6, 7.0}};
	assert(fa1.p[0].i == 0 && fa1.p[1].i == 4 && dblEq(fa1.p[1].d, 5.0) && fa1.p[2].i == 5);
	assert(fa1.p[3].i == 6 && fa1.p[4].i == 7 && da_count(fa1) == 8);

	printf("\n############ cap = %d\n", (int)da_capacity(fa1));
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);
	da_setn(fa1, 3, fs, 5);
	printf("\n############ cap = %d\n", (int)da_capacity(fa1));
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);

	// make sure that fast delete works if after the deleted area
	// there are less than n elements left
	da_deletenfast(fa1, 4, 3);
	// fa1.p[5] is not really valid anymore, but should be untouched
	assert(da_count(fa1) == 5 && fa1.p[4].i == 7 && fa1.p[3].i == 3 && fa1.p[5].i == 5);

	printf("\n############ cap = %d\n", (int)da_capacity(fa1));
	for(int i=0; i<da_count(fa1); ++i)
		printf("# %d: (%d %f)\n", i, fa1.p[i].i, fa1.p[i].d);

	da_free(fa1);
	da_free(fa2);
}

int main(int argc, char** argv)
{
	testint();
	testfoo();

	// if we got this far w/o assertion, things are good.
	printf("success!\n");

	return 0;
}
