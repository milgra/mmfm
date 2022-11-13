#ifndef mt_memory_h
#define mt_memory_h

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO separate unit tests */
/* TODO rethink debugging, current file and line numbers say nothing, cannot investigate double release */

#define CAL(X, Y, Z) mt_memory_calloc(X, Y, Z);
#define RET(X) mt_memory_retain(X)
#define REL(X) mt_memory_release(X)
#define HEAP(X) mt_memory_stack_to_heap(sizeof(X), NULL, NULL, (uint8_t*) &X)

struct mt_memory_head
{
#ifdef DEBUG
    uint32_t index; // allocation index for debugging/statistics
#endif
    void (*destructor)(void*);
    void (*descriptor)(void*, int);
    int32_t retaincount;
};

void*  mt_memory_alloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_calloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int));
void*  mt_memory_realloc(void* pointer, size_t size);
void*  mt_memory_retain(void* pointer);
char   mt_memory_release(void* pointer);
char   mt_memory_release_each(void* first, ...);
size_t mt_memory_retaincount(void* pointer);
void*  mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), uint8_t* data);
void   mt_memory_describe(void* pointer, int level);
#ifdef DEBUG
void mt_memory_stats();
#endif

#endif

#if __INCLUDE_LEVEL__ == 0

#include <execinfo.h>
#include <string.h>

#ifdef DEBUG

    #define MT_MAX_BLOCKS 10000000        /* max heads to store */
    #define MT_TRACEROUTE_RETAIN_INDEX 0  /* head index to stop at error */
    #define MT_TRACEROUTE_RELEASE_INDEX 0 /* head index to stop at error */

struct mt_memory_head* mt_memory_heads[MT_MAX_BLOCKS + 1] = {0};
uint32_t               mt_memory_index                    = 1; /* live object counter for debugging */

#endif

void* mt_memory_alloc(size_t size,                    /* size of data to store */
		      void (*destructor)(void*),      /* optional destructor */
		      void (*descriptor)(void*, int)) /* optional descriptor for describing memory map*/
{
    uint8_t* bytes = malloc(sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

#ifdef DEBUG
	head->index                      = mt_memory_index;
	mt_memory_heads[mt_memory_index] = head;
	if (head->index == MT_TRACEROUTE_RETAIN_INDEX || head->retaincount <= 0)
	{
	    printf("*** %i ALLOC : %i ***\n", head->index, head->retaincount);
	    void*  array[128];
	    int    size    = backtrace(array, 128);
	    char** strings = backtrace_symbols(array, size);
	    int    i;

	    for (i = 0; i < size; ++i) printf("%s\n", strings[i]);
	    free(strings);
	}
	mt_memory_index++;
#endif

	return bytes + sizeof(struct mt_memory_head);
    }
    else return NULL;
}

void* mt_memory_calloc(size_t size,                    /* size of data to store */
		       void (*destructor)(void*),      /* optional destructor */
		       void (*descriptor)(void*, int)) /* optional descriptor for describing memory map*/
{
    uint8_t* bytes = calloc(1, sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
    {
	struct mt_memory_head* head = (struct mt_memory_head*) bytes;

	head->destructor  = destructor;
	head->descriptor  = descriptor;
	head->retaincount = 1;

#ifdef DEBUG
	head->index                      = mt_memory_index;
	mt_memory_heads[mt_memory_index] = head;
	if (head->index == MT_TRACEROUTE_RETAIN_INDEX || head->retaincount <= 0)
	{
	    printf("*** %i CALLOC : %i ***\n", head->index, head->retaincount);
	    void*  array[128];
	    int    size    = backtrace(array, 128);
	    char** strings = backtrace_symbols(array, size);
	    int    i;

	    for (i = 0; i < size; ++i) printf("%s\n", strings[i]);
	    free(strings);
	}
	mt_memory_index++;
#endif

	return bytes + sizeof(struct mt_memory_head);
    }
    else return NULL;
}

void* mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), uint8_t* data)
{
    uint8_t* bytes = mt_memory_alloc(size, destructor, descriptor);

    if (bytes != NULL)
    {
	memcpy(bytes, data, size);
	return bytes;
    }
    else return NULL;
}

void* mt_memory_realloc(void* pointer, size_t size)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    bytes = realloc(bytes, sizeof(struct mt_memory_head) + size);
    if (bytes != NULL)
	return bytes + sizeof(struct mt_memory_head);
    else return NULL;
}

void* mt_memory_retain(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    if (head->retaincount < SIZE_MAX)
    {
	head->retaincount += 1;
#ifdef DEBUG
	if (head->index == MT_TRACEROUTE_RETAIN_INDEX)
	{
	    printf("*** %i RETAIN : %i ***\n", head->index, head->retaincount);
	    void*  array[128];
	    int    size    = backtrace(array, 128);
	    char** strings = backtrace_symbols(array, size);
	    int    i;

	    for (i = 0; i < size; ++i) printf("%s\n", strings[i]);
	    free(strings);
	}
#endif
	return pointer;
    }
    else return NULL;
}

char mt_memory_release(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

#ifdef DEBUG
    if (head->index == MT_TRACEROUTE_RELEASE_INDEX || head->retaincount <= 0)
    {
	printf("*** %i RELEASE : %i ***\n", head->index, head->retaincount - 1);
	void*  array[128];
	int    size    = backtrace(array, 128);
	char** strings = backtrace_symbols(array, size);
	int    i;

	for (i = 0; i < size; ++i) printf("%s\n", strings[i]);
	free(strings);
    }
#endif

    assert(head->retaincount > 0);

    head->retaincount -= 1;

    if (head->retaincount == 0)
    {
	if (head->destructor != NULL) head->destructor(pointer);
#ifndef DEBUG
	/* don't clean up to catch overrelease or leaks */
	free(bytes);
#endif
	return 1;
    }

    return 0;
}

char mt_memory_releaseeach(void* first, ...)
{
    va_list ap;
    void*   actual;
    char    released = 1;
    va_start(ap, first);
    for (actual = first; actual != NULL; actual = va_arg(ap, void*))
    {
	released &= mt_memory_release(actual);
    }
    va_end(ap);
    return released;
}

size_t mt_memory_retaincount(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    return head->retaincount;
}

void mt_memory_describe(void* pointer, int level)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    if (head->descriptor != NULL)
    {
	head->descriptor(pointer, ++level);
    }
    else
    {
	printf("no descriptor");
    }
}

#ifdef DEBUG

void mt_memory_stats()
{
    printf("\n***MEM STATS***\n");

    // print block statistics

    for (int index = 1; index < mt_memory_index; index++)
    {
	if (MT_TRACEROUTE_RELEASE_INDEX == 0 || MT_TRACEROUTE_RETAIN_INDEX == index)
	{
	    if (mt_memory_heads[index]->retaincount < 0) printf("OVERRELEASE at %i : %i\n", index, mt_memory_heads[index]->retaincount);
	    if (mt_memory_heads[index]->retaincount > 0) printf("LEAK at %i : %i\n", index, mt_memory_heads[index]->retaincount);
	}
    }
}

#endif

#endif
