# Snippets

Some standalone source files that don't deserve their own repositories.

| File                          | Description    |
|-------------------------------|----------------|
| [**DG_misc.h**](/DG_misc.h) | A public domain single-header C/C++ library with some useful functions to get the path/dir/name of the current executable and misc. string operations that are not available on all platforms - [***List of Functions***]( #list-of-functions-in-dg_misch) |
| [**DG_dynarr.h**](/DG_dynarr.h) | A public domain single-header library providing typesafe dynamic arrays for *plain C*, kinda like C++ std::vector (works with C++, but only with "simple" types) - [***Usage Example and List of Functions***]( #example-and-list-of-functions-for-dg_dynarrh) |
| [**SDL_stbimage.h**](/SDL_stbimage.h) | A public domain header-only C/C++ library for converting images to [SDL2](http://libsdl.org) `SDL_Surface*` using [stb_image.h](https://github.com/nothings/stb) - [***List of Functions***]( #list-of-functions-in-sdl_stbimageh) |
| [**sdl2_scancode_to_dinput.h**](/sdl2_scancode_to_dinput.h) | One static C array that maps SDL2 scancodes to Direct Input keynums (values of those DIK_* constants) - also public domain. |
| [**ImgToC.c**](/ImgToC.c) | Commandline tool converting images to .c files with a struct containing the image data. Same format as Gimp's "Export as .c" feature. Needs [stb_image.h](https://github.com/nothings/stb/) |

## List of functions in [**DG_misc.h**](/DG_misc.h)

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

## List of functions in [**SDL_stbimage.h**](/SDL_stbimage.h)

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


//   If you're gonna use SDL_Renderer, the following convenience functions
//   create SDL_Texture directly

// loads the image file at the given path into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Texture* STBIMG_LoadTexture(SDL_Renderer* renderer, const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Texture*
STBIMG_LoadTextureFromMemory(SDL_Renderer* renderer, const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Texture from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Texture* STBIMG_LoadTexture_RW(SDL_Renderer* renderer, SDL_RWops* src, int freesrc);



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

// Creates an SDL_Texture* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_Texture* STBIMG_CreateTexture(SDL_Renderer* renderer, const unsigned char* pixelData,
                                  int width, int height, int bytesPerPixel);


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

## Example and List of functions for [**DG_dynarr.h**](/DG_dynarr.h)

### Usage Example

```c
#define DG_DYNARR_IMPLEMENTATION // this define is only needed in *one* .c/.cpp file!
#include "DG_dynarr.h"

DA_TYPEDEF(int, MyIntArrType); // creates MyIntArrType - a dynamic array for ints

void printIntArr(MyIntArrType* arr, const char* name)
{
    // note that arr is a pointer here, so use *arr in the da_*() functions.
    printf("%s = {", name);
    if(da_count(*arr) > 0)
        printf(" %d", arr->p[0]);
    for(int i=1; i<da_count(*arr); ++i)
        printf(", %d", arr->p[i]);
    printf(" }\n");
}

void myFunction()
{
    MyIntArrType a1 = {0}; // make sure to zero out the struct
    // instead of = {0}; you could also call da_init(a1);

    da_push(a1, 42);
    assert(da_count(a1) == 1 && a1.p[0] == 42);

    int* addedElements = da_addn_uninit(a1, 3);
    assert(da_count(a1) == 4);
    for(size_t i=0; i<3; ++i)
        addedElements[i] = i+5;

    printIntArr(&a1, "a1"); // "a1 = { 42, 5, 6, 7 }"

    MyIntArrType a2;
    da_init(a2);

    da_addn(a2, a1.p, da_count(a1)); // copy all elements from a1 to a2
    assert(da_count(a2) == 4);

    da_insert(a2, 1, 11);
    printIntArr(&a2, "a2"); // "a2 = { 42, 11, 5, 6, 7 }"

    da_delete(a2, 2);
    printIntArr(&a2, "a2"); // "a2 = { 42, 11, 6, 7 }"

    da_deletefast(a2, 0);
    printIntArr(&a2, "a2"); // "a2 = { 7, 11, 6 }"

    da_push(a1, 3);
    printIntArr(&a1, "a1"); // "a1 = { 42, 5, 6, 7, 3 }"

    int x=da_pop(a1);
    printf("x = %d\n", x);  // "x = 3"
    printIntArr(&a1, "a1"); // "a1 = { 42, 5, 6, 7 }"
    
    da_free(a1); // make sure not to leak memory!
    da_free(a2);
}
```

### List of Functions (macros, really)

Note that for each `da_foo()` function there is a `dg_dynarr_foo()` equivalent
that you can use in case the short form name collides with other names your project uses.
`#define DG_DYNARR_NO_SHORTNAMES` before `#include "DG_dynarr.h"` disables the short versions.  

One thing to keep in mind is that, because of using macros, the arguments to
the "functions" are usually evaluated more than once, so you should avoid putting
things with side effect (like function-calls with side effects or `i++`) into them.
Notable exceptions are the value arguments (`v`) of `da_push()`, `da_set()`
and `da_insert()`, so it's still ok to do `da_push(arr, fun_with_sideffects());`
or `da_insert(a, 3, x++);`.

This library is inteded to be used with ***plain C*** code, but it also works with *C++*; however you
should only use it with "simple" types that can be moved around with `memcpy()`, i.e. don't
require copy- or move-constructors or destructors to work correctly.
So if you're writing C code that should also compile as C++ use this, but if you're
writing "real" C++ code, use `std::vector` or something else instead.

Furthermore, by default some sanity checks (like "is idx valid?") are done using
assertions and by returning NULL/doing nothing on error if possible.  
As this has some overhead, the behavior can be controlled with
`#define DG_DYNARR_INDEX_CHECK_LEVEL [0-3]`; see [DG_dynarr.h](/DG_dynarr.h) for details.

In this function reference:
* `T` refers to the element type of the array (usually used to indicate the return value type of some functions)
* `a` is always the dynamic array the function should operate on (of a type created with `DA_TYPEDEF`)
* `v` is always a single value that can be assigned to the array's element type
* `vals` is an array of values that can be assigned
* `idx` is always an integer (int, size_t, whatever, but must be `>= 0`) referring to a (0-based) index into the array
* `n` is always a positive integer, used to specify a number of elements

In addition to these functions, you can always access the `i`-th element of an
array `arr` by using `arr.p[i]`.

```c
// this macro is used to create an array type (struct) for elements of TYPE
// use like DA_TYPEDEF(int, MyIntArrType); MyIntArrType ia = {0}; da_push(ia, 42); ...
DA_TYPEDEF(TYPE, NewArrayTypeName)

// makes sure the array is initialized and can be used.
// either do YourArray arr = {0}; or YourArray arr; da_init(arr);
void da_init(a)

/*
 * This allows you to provide an external buffer that'll be used as long as it's big enough
 * once you add more elements than buf can hold, fresh memory will be allocated on the heap
 * Use like:
 * DA_TYPEDEF(double, MyDoubleArrType);
 * MyDoubleArrType arr;
 * double buf[8];
 * dg_dynarr_init_external(arr, buf, 8);
 * dg_dynarr_push(arr, 1.23);
 * ...
 */
void da_init_external(a, T* buf, size_t buf_cap)

// use this to free the memory allocated by dg_dynarr once you don't need the array anymore
// Note: it is safe to add new elements to the array after da_free()
//       it will allocate new memory, just like it would directly after da_init()
void da_free(a)


// add an element to the array (appended at the end)
void da_push(a, v)

// same as da_push(), just for consistency with addn (like insert and insertn)
void da_add(a, v)

// append n elements to a and initialize them from array vals, doesn't return anything
// ! vals (and all other args) are evaluated multiple times !
void da_addn(a, vals, n)

// add n elements to the end of the array and zeroes them with memset()
// returns pointer to first added element, NULL if out of memory (array is empty then)
T* da_addn_zeroed(a, n)

// add n elements to the end of the array, will remain uninitialized
// returns pointer to first added element, NULL if out of memory (array is empty then)
T* da_addn_uninit(a, n)


// insert a single value v at index idx
void da_insert(a, idx, v)

// insert n elements into a at idx, initialize them from array vals
// doesn't return anything
// ! vals (and all other args) is evaluated multiple times ! 
void da_insertn(a, idx, vals, n)

// insert n elements into a at idx and zeroe them with memset() 
// returns pointer to first inserted element or NULL if out of memory
T* da_insertn_zeroed(a, idx, n)

// insert n uninitialized elements into a at idx;
// returns pointer to first inserted element or NULL if out of memory
T* da_insertn_uninit(a, idx, n) 


// set a single value v at index idx - like "a.p[idx] = v;" but with checks (unless disabled)
T da_set(a, idx, v)

// overwrite n elements of a, starting at idx, with values from array vals
// doesn't return anything
// ! vals (and all other args) is evaluated multiple times ! 
void da_setn(a, idx, vals, n)


// delete the element at idx, moving all following elements (=> keeps order)
void da_delete(a, idx)

// delete n elements starting at idx, moving all following elements (=> keeps order)
void da_deleten(a, idx, n) 

// delete the element at idx, move the last element there (=> doesn't keep order)
void da_deletefast(a, idx)

// delete n elements starting at idx, move the last n elements there (=> doesn't keep order)
void da_deletenfast(a, idx, n)

// removes all elements from the array, but does not free the buffer
// (if you want to free the buffer too, just use da_free())
void da_clear(a)

// sets the logical number of elements in the array
// if n > dg_dynarr_count(a), the logical count will be increased accordingly
// and the new elements will be uninitialized
// returns new count if successful, else 0
size_t da_setcount(a, n)

// make sure the array can store n elements without reallocating
// logical count remains unchanged
void da_reserve(a, n)


// this makes sure a only uses as much memory as for its elements
// => maybe useful if a used to contain a huge amount of elements,
//    but you deleted most of them and want to free some memory
// Note however that this implies an allocation and copying the remaining
// elements, so only do this if it frees enough memory to be worthwhile!
void da_shrink_to_fit(a)

// removes and returns the last element of the array
T da_pop(a)


// returns the last element of the array
T da_last(a)

// returns the pointer *to* the last element of the array
// (in contrast to dg_dynarr_end() which returns a pointer *after* the last element)
// returns NULL if array is empty
T* da_lastptr(a)

// get element at index idx (like a.p[idx]), but with checks
// (unless you disabled them with #define DG_DYNARR_INDEX_CHECK_LEVEL 0)
T da_get(a, idx)

// get pointer to element at index idx (like &a.p[idx]), but with checks
// and it returns NULL if idx is invalid
T* da_getptr(a, idx)

// returns a pointer to the first element of the array
// (together with dg_dynarr_end() you can do C++-style iterating)
T* da_begin(a)

// returns a pointer to the past-the-end element of the array
// Allows C++-style iterating, in case you're into that kind of thing:
// for(T *it=da_begin(a), *end=da_end(a); it!=end; ++it) foo(*it);
// (see da_lastptr() to get a pointer *to* the last element)
T* da_end(a)


// returns (logical) number of elements currently in the array
size_t da_count(a)

// get the current reserved capacity of the array
size_t da_capacity(a)

// returns 1 if the array is empty, else 0
bool da_empty(a)

// returns 1 if the last (re)allocation when inserting failed (Out Of Memory)
//   or if the array has never allocated any memory yet, else 0
// deleting the contents when growing fails instead of keeping old may seem
// a bit uncool, but it's simple and OOM should rarely happen on modern systems
// anyway - after all you need to deplete both RAM and swap/pagefile.sys
bool da_oom(a)


// sort a using the given qsort()-comparator cmp
// (just a slim wrapper around qsort())
void da_sort(a, cmp)
```
