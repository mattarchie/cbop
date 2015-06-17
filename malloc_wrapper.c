#include "dmmalloc.h"
#include "malloc_wrapper.h"
#undef NDEBUG
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <malloc.h>

//http://stackoverflow.com/questions/262439/create-a-wrapper-function-for-malloc-and-free-in-c

//prototypes for the dlsym using calloc workaround
void* tempcalloc(size_t, size_t);
static inline void calloc_init();

static void *(*libc_malloc)(size_t) = NULL;
static void *(*libc_realloc)(void*, size_t) = NULL;
static void (*libc_free)(void*) = NULL;
static void *(*libc_calloc)(size_t, size_t) = NULL;
static size_t (*libc_malloc_usable_size)(void*) = NULL;
static void *(*calloc_func)(size_t, size_t) = tempcalloc; //part of dlsym workaround

#define CHARSIZE 100
#define VISUALIZE
static char calloc_hack[CHARSIZE];
static short initializing = 0;
//unsupported malloc operations are aborted immediately
void* memalign(size_t size, size_t boundary){
	printf("\nUNSUPPORTED OPERATION memalign\n");
	abort();
}
void* aligned_alloc(size_t size, size_t boundary){
	printf("\nUNSUPPORTED OPERATION: aligned_alloc\n");
	abort();
	return NULL;
}
void* valloc(size_t size){
	printf("\nUNSUPPORTED OPERATION: valloc\n");
	abort();
	return NULL;
}
struct mallinfo mallinfo(){
	printf("\nUNSUPPORTED OPERATION: mallinfo\n");
	abort();
}
//Supported allocation functions
int posix_memalign(void** dest_ptr, size_t align, size_t size){
#ifdef VISUALIZE
	printf("p");
	fflush(stdout);
#endif
	int ones = __builtin_popcount (align);
	if(ones != 1)
		return -1; //not power of 2
	void* dmm = aligned_alloc(align, size);
	if(dmm == NULL)
		return -1; //REAL ERROR???
	*dest_ptr = dmm;
	return 0;
}
void* malloc(size_t s){
#ifdef VISUALIZE
	printf("+");
	fflush(stdout);
#endif
	void* p = dm_malloc(s);
	assert (p != NULL);
	return p;
}
void* realloc(void *p , size_t s){
#ifdef VISUALIZE
	printf(".");
	fflush(stdout);
#endif
	assert (p != calloc_hack);
	void* p2 = dm_realloc(p, s);
	assert (p2!=NULL);
	return p2;
}
void free(void * p){
#ifdef VISUALIZE
	printf("-");
	fflush(stdout);
#endif
	if(p == NULL || p == calloc_hack) return;
	dm_free(p);
}

size_t malloc_usable_size(void* ptr){
#ifdef VISUALIZE
	printf("s");
	fflush(stdout);
#endif
	size_t size = dm_malloc_usable_size(ptr);
	assert(size > 0);
	return size;
}


void * calloc(size_t sz, size_t n){
#ifdef VISUALIZE
	printf("0");
	fflush(stdout);
#endif
	calloc_init();
	assert(calloc_func != NULL);
	void* p = calloc_func(sz, n);
	assert (p!=NULL);
	return p;
}

static inline void calloc_init(){
	if(libc_calloc == NULL && !initializing){
		//first allocation
		initializing = 1; //don't recurse
		calloc_func = tempcalloc;
		libc_calloc = dlsym(RTLD_NEXT, "calloc");
		assert(libc_calloc != NULL);
		calloc_func = dm_calloc;
	}
}
void* tempcalloc(size_t s, size_t n){
	if(s * n > CHARSIZE){
		abort();
		return NULL;
	}
	int i;
	for(i = 0; i < CHARSIZE; i++)
		calloc_hack[i] = '\0';
	return calloc_hack;
}

inline void * sys_malloc(size_t s){
	if(libc_malloc == NULL)
		libc_malloc = dlsym(RTLD_NEXT, "malloc");
	assert(libc_malloc != NULL);
    return libc_malloc(s);
}
inline void * sys_realloc(void * p , size_t size){
	if(p == calloc_hack){
		void* payload = sys_malloc(size);
		payload = memcpy(payload, p, CHARSIZE);
		assert(payload != NULL);
		return payload;
	}
	if(libc_realloc == NULL)
		libc_realloc = dlsym(RTLD_NEXT, "realloc");
	assert(libc_realloc != NULL);
	void* p2 = libc_realloc(p, size);
	assert(p2 != NULL);
	return p2;
}
inline void sys_free(void * p){
	if(p == NULL || p == calloc_hack) return;
	if(libc_free == NULL)
		libc_free = dlsym(RTLD_NEXT, "free");
	assert(libc_free != NULL);
    libc_free(p);
}
inline size_t sys_malloc_usable_size(void* p){
	if(libc_malloc_usable_size == NULL)
		libc_malloc_usable_size = dlsym(RTLD_NEXT, "malloc_usable_size");
	assert (libc_malloc_usable_size != NULL);
	return libc_malloc_usable_size(p);
}
inline void * sys_calloc(size_t s, size_t n){
	calloc_init();
	assert(libc_calloc != NULL);
    void* p = libc_calloc(s, n);
    assert (p!=NULL);
	return p;
}

#undef CHARSIZE