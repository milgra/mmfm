#ifndef mt_memory_h
#define mt_memory_h

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* TODO separate unit tests */
/* TODO rethink debugging, current file and line numbers say nothing, cannot investigate double release */

#define CAL(X, Y, Z) mt_memory_calloc(X, Y, Z, __FILE__, __LINE__);
#define RET(X) mt_memory_retain(X, __FILE__, __LINE__)
#define REL(X) mt_memory_release(X, __FILE__, __LINE__)
#define HEAP(X) mt_memory_stack_to_heap(sizeof(X), NULL, NULL, (uint8_t*) &X, __FILE__, __LINE__)

struct mt_memory_head
{
    char id[2]; // starting bytes for mt_memory managed memory ranges to detect invalid object during retain/release
#ifdef DEBUG
    uint32_t index; // allocation index for debugging/statistics
#endif
    void (*destructor)(void*);
    void (*descriptor)(void*, int);
    size_t retaincount;
};

void*  mt_memory_alloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), char* file, int line);
void*  mt_memory_calloc(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), char* file, int line);
void*  mt_memory_realloc(void* pointer, size_t size);
void*  mt_memory_retain(void* pointer, char* file, int line);
char   mt_memory_release(void* pointer, char* file, int line);
char   mt_memory_release_each(void* first, ...);
size_t mt_memory_retaincount(void* pointer);
void*  mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), uint8_t* data, char* file, int line);
void   mt_memory_describe(void* pointer, int level);
void   mt_memory_exit(char* text, char* file, int line);
#ifdef DEBUG
void mt_memory_stat(void* pointer);
void mt_memory_stats();
#endif

#endif

#if __INCLUDE_LEVEL__ == 0

#include <string.h>

#ifdef DEBUG

    #define MT_MAX_BLOCKS 10000000
    #define MT_TRACEROUTE_ALLOC 0
    #define MT_TRACEROUTE_CALLOC 0
    #define MT_TRACEROUTE_RETAIN 0

struct mt_memory_info
{
    char  live;
    char  stat;
    char* file;
    int   line;
    void* ptr;
};

struct mt_memory_info mt_memory_infos[MT_MAX_BLOCKS + 1] = {0};
uint32_t              mt_memory_index                    = 1; /* live object counter for debugging */

#endif

void* mt_memory_alloc(size_t size,                    /* size of data to store */
		      void (*destructor)(void*),      /* optional destructor */
		      void (*descriptor)(void*, int), /* optional descriptor for describing memory map*/
		      char* file,                     /* caller file name */
		      int   line)                       /* caller file line number */
{
    if (size == 0) mt_memory_exit("Trying to allocate 0 bytes for", file, line);
    uint8_t* bytes = malloc(sizeof(struct mt_memory_head) + size);
    if (bytes == NULL) mt_memory_exit("Out of RAM \\_(o)_/ for", file, line);

    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    head->id[0]       = 'z';
    head->id[1]       = 'c';
    head->destructor  = destructor;
    head->descriptor  = descriptor;
    head->retaincount = 1;

#ifdef DEBUG
    head->index                           = mt_memory_index;
    mt_memory_infos[mt_memory_index].live = 1;
    mt_memory_infos[mt_memory_index].file = file;
    mt_memory_infos[mt_memory_index].line = line;
    mt_memory_infos[mt_memory_index].ptr  = bytes + sizeof(struct mt_memory_head);
    if (mt_memory_index == MT_TRACEROUTE_ALLOC) abort();
    if (mt_memory_index == MT_MAX_BLOCKS) printf("INCREASE MT_MAX_BLOCKS COUNT IN MT_MEMORY\n");
    mt_memory_index++;
#endif

    return bytes + sizeof(struct mt_memory_head);
}

void* mt_memory_calloc(size_t size,                    /* size of data to store */
		       void (*destructor)(void*),      /* optional destructor */
		       void (*descriptor)(void*, int), /* optional descriptor for describing memory map*/
		       char* file,                     /* caller file name */
		       int   line)                       /* caller file line number */
{
    if (size == 0) mt_memory_exit("Trying to allocate 0 bytes for", file, line);
    uint8_t* bytes = calloc(1, sizeof(struct mt_memory_head) + size);
    if (bytes == NULL) mt_memory_exit("Out of RAM \\_(o)_/ for", file, line);

    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    head->id[0]       = 'z';
    head->id[1]       = 'c';
    head->destructor  = destructor;
    head->descriptor  = descriptor;
    head->retaincount = 1;

#ifdef DEBUG
    head->index                           = mt_memory_index;
    mt_memory_infos[mt_memory_index].live = 1;
    mt_memory_infos[mt_memory_index].file = file;
    mt_memory_infos[mt_memory_index].line = line;
    mt_memory_infos[mt_memory_index].ptr  = bytes + sizeof(struct mt_memory_head);
    if (mt_memory_index == MT_TRACEROUTE_CALLOC) abort();
    if (mt_memory_index == MT_MAX_BLOCKS) printf("INCREASE MT_MAX_BLOCKS COUNT IN MT_MEMORY\n");
    mt_memory_index++;
#endif

    return bytes + sizeof(struct mt_memory_head);
}

void* mt_memory_stack_to_heap(size_t size, void (*destructor)(void*), void (*descriptor)(void*, int), uint8_t* data, char* file, int line)
{
    uint8_t* bytes = mt_memory_alloc(size, destructor, descriptor, file, line);
    if (bytes == NULL) mt_memory_exit("Out of RAM \\_(o)_/ for", file, line);
    memcpy(bytes, data, size);
    return bytes;
}

void* mt_memory_realloc(void* pointer, size_t size)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    bytes = realloc(bytes, sizeof(struct mt_memory_head) + size);
    if (bytes == NULL) mt_memory_exit("Out of RAM \\_(o)_/ when realloc", "", 0);

    return bytes + sizeof(struct mt_memory_head);
}

void* mt_memory_retain(void* pointer, char* file, int line)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    // check memory range id
    assert(head->id[0] == 'z' && head->id[1] == 'c');

#ifdef DEBUG
    if (head->index == MT_TRACEROUTE_RETAIN) abort();
#endif

    head->retaincount += 1;
    if (head->retaincount == SIZE_MAX) mt_memory_exit("Maximum retain count reached \\(o)_/ for", "", 0);

    return pointer;
}

char mt_memory_release(void* pointer, char* file, int line)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    // check memory range id
    assert(head->id[0] == 'z' && head->id[1] == 'c');

    if (head->retaincount == 0) mt_memory_exit("Tried to release already released memory for", "", 0);

    head->retaincount -= 1;

    if (head->retaincount == 0)
    {
#ifdef DEBUG
	mt_memory_infos[head->index].live = 0;
#endif

	if (head->destructor != NULL) head->destructor(pointer);
	// zero out bytes that will be deallocated so it will be easier to detect re-using of released mt_memory areas
	memset(bytes, 0, sizeof(struct mt_memory_head));
	free(bytes);

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
	released &= mt_memory_release(actual, __FILE__, __LINE__);
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

    // check memory range id
    assert(head->id[0] == 'z' && head->id[1] == 'c');

    return head->retaincount;
}

void mt_memory_describe(void* pointer, int level)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    // check memory range id
    assert(head->id[0] == 'z' && head->id[1] == 'c');

    if (head->descriptor != NULL)
    {
	head->descriptor(pointer, ++level);
    }
    else
    {
	printf("no descriptor");
    }
}

void mt_memory_exit(char* text, char* file, int line)
{
    printf("%s %s %i\n", text, file, line);
    exit(EXIT_FAILURE);
}

#ifdef DEBUG

void mt_memory_stat(void* pointer)
{
    assert(pointer != NULL);

    uint8_t* bytes = (uint8_t*) pointer;
    bytes -= sizeof(struct mt_memory_head);
    struct mt_memory_head* head = (struct mt_memory_head*) bytes;

    // check memory range id
    assert(head->id[0] == 'z' && head->id[1] == 'c');

    printf("mem stat %s %i", mt_memory_infos[head->index].file, mt_memory_infos[head->index].line);
}

void mt_memory_stats()
{
    printf("\n***MEM STATS***\n");

    // print block statistics

    for (int index = 1; index < mt_memory_index; index++)
    {
	if (mt_memory_infos[index].stat == 0)
	{
	    char* file  = mt_memory_infos[index].file;
	    int   line  = mt_memory_infos[index].line;
	    int   count = 0;
	    for (int i = 0; i < mt_memory_index; i++)
	    {
		if (mt_memory_infos[i].stat == 0 && mt_memory_infos[i].file == file && mt_memory_infos[i].line == line)
		{
		    mt_memory_infos[i].stat = 1;
		    count++;
		}
	    }
	    printf("%s %i block count : %i\n", file, line, count);
	}
    }

    // print leak statistics

    uint32_t count = 0;
    for (int index = 1; index < mt_memory_index; index++)
    {
	if (mt_memory_infos[index].live)
	{
	    printf("unreleased block %i at %s %i desc : \n", index, mt_memory_infos[index].file, mt_memory_infos[index].line);
	    // mt_memory_describe(mt_memory_infos[index].ptr, 0);
	    printf("\n");
	    count++;
	}
    }

    printf("total unreleased blocks : %u\n", count);
}

#endif

#endif
