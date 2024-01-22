#ifndef _MEMORY_ALLOC_H_
#define _MEMORY_ALLOC_H_

void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* p);
void operator delete[](void* p);
void* mem_alloc(size_t size);
void mem_free(void* p);

#endif