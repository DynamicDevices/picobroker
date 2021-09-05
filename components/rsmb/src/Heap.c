/*******************************************************************************
 * Copyright (c) 2007, 2013 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *******************************************************************************/


/**
 * @file
 * \brief functions to manage the heap with the goal of eliminating memory leaks
 *
 * For any module to use these functions transparently, simply include the Heap.h
 * header file.  Malloc and free will be redefined, but will behave in exactly the same
 * way as normal, so no recoding is necessary.
 *
 * */

#include "Tree.h"
#include "Log.h"
#include "StackTrace.h"
char* Broker_recordFFDC(char* symptoms);

#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "Heap.h"

#undef malloc
#undef realloc
#undef free

static heap_info state = {0, 0}; /**< global heap state information */
static int eyecatcher = 0x88888888;

/**
 * Each item on the heap is recorded with this structure.
 */
typedef struct
{
	char* file;		/**< the name of the source file where the storage was allocated */
	int line;		/**< the line no in the source file where it was allocated */
	void* ptr;		/**< pointer to the allocated storage */
	int size;       /**< size of the allocated storage */
} storageElement;

static Tree heap;	/**< Tree that holds the allocation records */
static char* errmsg = "Memory allocation error";

/**
 * Round allocation size up to a multiple of the size of an int.  Apart from possibly reducing fragmentation,
 * on the old v3 gcc compilers I was hitting some weird behaviour, which might have been errors in
 * sizeof() used on structures and related to packing.  In any case, this fixes that too.
 * @param size the size actually needed
 * @return the rounded up size
 */
int roundup(int size)
{
	static int multsize = 4*sizeof(int);

	if (size % multsize != 0)
		size += multsize - (size % multsize);
	return size;
}


/**
 * List callback function for comparing storage elements
 * @param a pointer to the current content in the tree (storageElement*)
 * @param b pointer to the memory to free
 * @return boolean indicating whether a and b are equal
 */
int ptrCompare(void* a, void* b, int value)
{
	a = ((storageElement*)a)->ptr;
	if (value)
		b = ((storageElement*)b)->ptr;

	return (a > b) ? -1 : (a == b) ? 0 : 1;
}


void Heap_check(char* string, void* ptr)
{
	Node* curnode = NULL;
	storageElement* prev, *s = NULL;

	printf("Heap_check start %p\n", ptr);
	while ((curnode = TreeNextElement(&heap, curnode)) != NULL)
	{
		prev = s;
		s = (storageElement*)(curnode->content);

		if (prev)
		{
		if (ptrCompare(s, prev, 1) != -1)
		{
			printf("%s: heap order error %d %p %p\n", string, ptrCompare(s, prev, 1), prev->ptr, s->ptr);
			exit(99);
		}
		else
			printf("%s: heap order good %d %p %p\n", string, ptrCompare(s, prev, 1), prev->ptr, s->ptr);
		}
	}
}


/**
 * Allocates a block of memory.  A direct replacement for malloc, but keeps track of items
 * allocated in a list, so that free can check that a item is being freed correctly and that
 * we can check that all memory is freed at shutdown.
 * @param file use the __FILE__ macro to indicate which file this item was allocated in
 * @param line use the __LINE__ macro to indicate which line this item was allocated at
 * @param size the size of the item to be allocated
 * @return pointer to the allocated item, or NULL if there was an error
 */
void* mymalloc(char* file, int line, size_t size)
{
	storageElement* s = NULL;
	int space = sizeof(storageElement);
	int filenamelen = strlen(file)+1;

	size = roundup(size);
	if ((s = malloc(sizeof(storageElement))) == NULL)
	{
		Log(LOG_ERROR, 13, errmsg);
		return NULL;
	}
	s->size = size; /* size without eyecatchers */
	if ((s->file = malloc(filenamelen)) == NULL)
	{
		Log(LOG_ERROR, 13, errmsg);
		free(s);
		return NULL;
	}
	space += filenamelen;
	strcpy(s->file, file);
	s->line = line;
	/* Add space for eyecatcher at each end */
	if ((s->ptr = malloc(size + 2*sizeof(int))) == NULL)
	{
		Log(LOG_ERROR, 13, errmsg);
		free(s->file);
		free(s);
		return NULL;
	}
	space += size + 2*sizeof(int);
	*(int*)(s->ptr) = eyecatcher; /* start eyecatcher */
	*(int*)(((char*)(s->ptr)) + (sizeof(int) + size)) = eyecatcher; /* end eyecatcher */
	//Log(LOG_DEBUG, "Allocating %d bytes in heap at file %s line %d ptr %p\n", size, file, line, s->ptr);
	TreeAdd(&heap, s, space);
	state.current_size += size;
	if (state.current_size > state.max_size)
		state.max_size = state.current_size;
	return ((int*)(s->ptr)) + 1;	/* skip start eyecatcher */
}


void checkEyecatchers(char* file, int line, void* p, int size)
{
	int *sp = (int*)p;
	char *cp = (char*)p;
	int us;
	static char* msg = "Invalid %s eyecatcher %d in heap item at file %s line %d";

	if ((us = *--sp) != eyecatcher)
		Log(LOG_SEVERE, 13, msg, "start", us, file, line);

	cp += size;
	if ((us = *(int*)cp) != eyecatcher)
		Log(LOG_SEVERE, 13, msg, "end", us, file, line);
}


/**
 * Frees a block of memory.  A direct replacement for free, but checks that a item is in
 * the allocates list first.
 * @param file use the __FILE__ macro to indicate which file this item was allocated in
 * @param line use the __LINE__ macro to indicate which line this item was allocated at
 * @param p pointer to the item to be freed
 */
void myfree(char* file, int line, void* p)
{
	Node* e = TreeFind(&heap, ((int*)p)-1);

	if (e == NULL)
		Log(LOG_SEVERE, 13, "Failed to remove heap item at file %s line %d", file, line);
	else
	{
		storageElement* s = (storageElement*)(e->content);
		//Log(LOG_DEBUG, "Freeing %d bytes in heap at file %s line %d, heap use now %d bytes\n",
											// s->size, file, line, state.current_size);
		checkEyecatchers(file, line, p, s->size);
		free(s->ptr);
		free(s->file);
		state.current_size -= s->size;
		TreeRemoveNodeIndex(&heap, e, 0);
		free(s);
	}
}


/**
 * Reallocates a block of memory.  A direct replacement for realloc, but keeps track of items
 * allocated in a list, so that free can check that a item is being freed correctly and that
 * we can check that all memory is freed at shutdown.
 * We have to remove the item from the tree, as the memory is in order and so it needs to
 * be reinserted in the correct place.
 * @param file use the __FILE__ macro to indicate which file this item was reallocated in
 * @param line use the __LINE__ macro to indicate which line this item was reallocated at
 * @param p pointer to the item to be reallocated
 * @param size the new size of the item
 * @return pointer to the allocated item, or NULL if there was an error
 */
void *myrealloc(char* file, int line, void* p, size_t size)
{
	void* rc = NULL;
	storageElement* s = TreeRemoveKey(&heap, ((int*)p)-1);

	if (s == NULL)
		Log(LOG_SEVERE, 13, "Failed to reallocate heap item at file %s line %d", file, line);
	else
	{
		int space = sizeof(storageElement);
		int filenamelen = strlen(file)+1;

		checkEyecatchers(file, line, p, s->size);
		size = roundup(size);
		state.current_size += size - s->size;
		if (state.current_size > state.max_size)
			state.max_size = state.current_size;
		if ((s->ptr = realloc(s->ptr, size + 2*sizeof(int))) == NULL)
		{
			Log(LOG_ERROR, 13, errmsg);
			return NULL;
		}
		space += size + 2*sizeof(int);
		*(int*)(s->ptr) = eyecatcher; /* start eyecatcher */
		*(int*)(((char*)(s->ptr)) + (sizeof(int) + size)) = eyecatcher; /* end eyecatcher */
		s->size = size;
		s->file = realloc(s->file, filenamelen);
		space += filenamelen;
		strcpy(s->file, file);
		s->line = line;
		rc = s->ptr;
		TreeAdd(&heap, s, space);
	}
	return (rc == NULL) ? NULL : ((int*)(rc)) + 1;	/* skip start eyecatcher */
}


/**
 * Scans the heap and reports any items currently allocated.
 * To be used at shutdown if any heap items have not been freed.
 */
void Heap_scan(FILE* file)
{
	Node* current = NULL;
	fprintf(file, "Heap scan start, total %d bytes\n", state.current_size);
	while ((current = TreeNextElement(&heap, current)) != NULL)
	{
		storageElement* s = (storageElement*)(current->content);
		fprintf(file, "Heap element size %d, line %d, file %s, ptr %p\n", s->size, s->line, s->file, s->ptr);
		fprintf(file, "  Content %*.s\n", (10 > current->size) ? s->size : 10, (char*)s->ptr);
	}
	fprintf(file, "Heap scan end\n");
}


/**
 * Heap initialization.
 */
int Heap_initialize()
{
	TreeInitializeNoMalloc(&heap, ptrCompare);
	heap.heap_tracking = 0; /* no recursive heap tracking! */
	return 0;
}


/**
 * Heap termination.
 */
void Heap_terminate()
{
	if (state.current_size > 0)
		Broker_recordFFDC("Some memory not freed at shutdown, possible memory leak");
}


/**
 * Access to heap state
 * @return pointer to the heap state structure
 */
heap_info* Heap_get_info()
{
	return &state;
}


/**
 * Dump a string from the heap so that it can be displayed conveniently
 * @param file file handle to dump the heap contents to
 * @param str the string to dump, could be NULL
 */
int HeapDumpString(FILE* file, char* str)
{
	int rc = 0;
	int len = str ? strlen(str) + 1 : 0; /* include the trailing null */

	if (fwrite(&(str), sizeof(char*), 1, file) != 1)
		rc = -1;
	else if (fwrite(&(len), sizeof(int), 1 ,file) != 1)
		rc = -1;
	else if (len > 0 && fwrite(str, len, 1, file) != 1)
		rc = -1;
	return rc;
}


/**
 * Dump the state of the heap
 * @param file file handle to dump the heap contents to
 */
int HeapDump(FILE* file)
{
	int rc = 0;
	Node* current = NULL;

	while (rc == 0 && ((current = TreeNextElement(&heap, current)) != NULL))
	{
		storageElement* s = (storageElement*)(current->content);
		void* ptr = ((int*)s->ptr) + 1; /* skip eye catcher */
		
		//printf("Heap element size %d %d, line %d, file %s, ptr %p\n", current->size, s->size, s->line, s->file, s->ptr);
		//printf("  Content %*.s\n", (10 > s->size) ? s->size : 10, (char*)s->ptr);
		if (fwrite(&(ptr), sizeof(ptr), 1, file) != 1)
			rc = -1;
		else if (fwrite(&(s->size), sizeof(s->size), 1, file) != 1)
			rc = -1;
		else if (fwrite(ptr, s->size, 1, file) != 1)
			rc = -1;
	}

	return rc;
}


#if defined(HEAP_UNIT_TESTS)

void Log(int log_level, int msgno, char* format, ...)
{
	printf("Log %s", format);
}

char* Broker_recordFFDC(char* symptoms)
{
	printf("recordFFDC");
	return "";
}

#define malloc(x) mymalloc(__FILE__, __LINE__, x)
#define realloc(a, b) myrealloc(__FILE__, __LINE__, a, b)
#define free(x) myfree(__FILE__, __LINE__, x)

int main(int argc, char *argv[])
{
	char* h = NULL;
	Heap_initialize();

	h = malloc(12);
	free(h);
	printf("freed h\n");

	h = malloc(12);
	h = realloc(h, 14);
	h = realloc(h, 25);
	h = realloc(h, 255);
	h = realloc(h, 2225);
	h = realloc(h, 22225);
    printf("freeing h\n");
	free(h);
	Heap_terminate();
	printf("Finishing\n");
	return 0;
}

#endif
