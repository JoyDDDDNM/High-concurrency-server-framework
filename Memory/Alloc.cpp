#include "MemoryMgr.hpp"
#include "Alloc.hpp"

void* operator new(size_t size) {
	return MemoryMgr::getInstance().allocMem(size);
}

void operator delete(void* p) {
	MemoryMgr::getInstance().freeMem(p);
}

void* operator new[](size_t size) {
	return MemoryMgr::getInstance().allocMem(size);
}

void operator delete[](void* p) {
	MemoryMgr::getInstance().freeMem(p);
}

void* mem_alloc(size_t size) {
	return malloc(size);
}

void mem_free(void* p) {
	free(p);
}
