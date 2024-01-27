#ifndef _MEMORYMGR_HPP_
#define _MEMORYMGR_HPP_

#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <mutex>

// maximum size of each memory block
#define MAX_MEMORY_SIZE 1024

// DEBUG print
#ifdef _DEBUG
	#include <stdio.h>
	#define xPrintf(...) printf( __VA_ARGS__ )
#else
	#define xPrintf(...)
#endif

class MemoryPool;

// header of each memory block
class MemoryBlock {
public:
	MemoryBlock();

	~MemoryBlock();

	// sequence number of block
	int nID;

	// number of user accessing this block
	int nRef;

	// the memory pool it belongs to
	MemoryPool* pPool;

	// pointer to next block
	MemoryBlock* pNext;

	// if the block is in pool
	bool bPool;
private:

	char padding[3];
};

// memory pool
class MemoryPool {
	public:
		MemoryPool();

		MemoryPool(size_t nSize, size_t nBlock);

		~MemoryPool();

		// request memroy and return it to manager
		void* allocMem(size_t nSize);

		// free memory
		void freeMem(void* pMem);

		void initMemory();
	private:
		// address of memory pool
		char* _pBuf;

		// head of meomory block
		MemoryBlock* _pHeader;

		// size of each block
		size_t _nSize;

		// number of blocks in pool
		size_t _nBlock;

		std::mutex _mutex;
};

// memory manager
class MemoryMgr {
	public:
		// only way to access memory manager
		static MemoryMgr& getInstance();

		// singleton object should not be copied
		MemoryMgr(MemoryMgr& mg) = delete;
		void operator=(const MemoryMgr&) = delete;

		// request memroy
		void* allocMem(size_t nSize);

		// free memory
		void freeMem(void* pMem);

		void addRef(void* pMem);

	private:
		// initialize mapping array
		void init(int nBegin, int nEnd, MemoryPool* pMemP);

		// avoid user access manager directly
		static MemoryMgr mgr;
		
		MemoryPool _pool64;
		MemoryPool _pool128;
		MemoryPool _pool256;
		MemoryPool _pool512;
		MemoryPool _pool1024;
		 
		// used to access the corresponding memory pool when request memory
		MemoryPool* _szAlloc[MAX_MEMORY_SIZE + 1]; 

		// stop user trying to initialize manager
		MemoryMgr();

		~MemoryMgr();
};

#endif