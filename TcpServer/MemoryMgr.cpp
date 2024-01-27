#include "MemoryMgr.hpp"

MemoryBlock::MemoryBlock() : pNext{ nullptr } {};

MemoryBlock::~MemoryBlock() {};

// memory pool
MemoryPool::MemoryPool() :_pBuf{ nullptr }, _pHeader{ nullptr }, _nSize{ 0 }, _nBlock{ 0 } {};

MemoryPool::MemoryPool(size_t nSize, size_t nBlock) :_pBuf{ nullptr }, _pHeader{ nullptr }, _nSize{ nSize }, _nBlock{ nBlock } {
	// get the size of pointer for memory alignment
	const size_t n = sizeof(void*);

	// resize memory block
	_nSize = (nSize / n) * n + (nSize % n ? n : 0);
};

MemoryPool::~MemoryPool() {
	if (_pBuf) free(_pBuf);
};

// request memroy and return it to manager
void* MemoryPool::allocMem(size_t nSize) {
	std::lock_guard<std::mutex> lock(_mutex);

	if (!_pBuf) initMemory();

	MemoryBlock* pRet{ nullptr };

	if (_pHeader == nullptr) {
		pRet = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
		pRet->bPool = false;
		pRet->nID = -1;
		pRet->nRef = 1;
		pRet->pPool = nullptr;
		pRet->pNext = nullptr;
	}
	else {
		pRet = _pHeader;
		_pHeader = _pHeader->pNext;
		assert(pRet->nRef == 0);
		pRet->nRef = 1;
	}

	xPrintf("allocMem: %llx, id=%d, size=%d \n", pRet, pRet->nID, nSize);
	// reset pointer to skip 
	return ((char*)pRet + sizeof(MemoryBlock));
}

// free memory
void MemoryPool::freeMem(void* pMem) {

	// pointer to the header of memory block
	MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));

	//assert(pBlock->nRef == 1);

	std::lock_guard<std::mutex> lock(_mutex);
	if (pBlock->nRef-- > 1) {
		// memory block is accessed by more than 1 server
		return;
	}

	if (pBlock->bPool) {
		// set the current block as the first block to be used next time, manage memory pool as linklist
		pBlock->pNext = _pHeader;
		_pHeader = pBlock;
	}
	else {
		// an extra memory not existing in memory pool
		free(pBlock);
	}
}

void MemoryPool::initMemory() {
	assert(_pBuf == nullptr);

	if (_pBuf) {
		std::cout << "Memory pool has been initialized" << std::endl;
		return;
	}

	// total size of pool
	size_t each_buf = _nSize + sizeof(MemoryBlock);
	size_t bufSize = each_buf * _nBlock;

	// request memory to initialize pool
	_pBuf = (char*)malloc(bufSize);

	_pHeader = (MemoryBlock*)_pBuf;
	_pHeader->bPool = true;
	_pHeader->nID = 0;
	_pHeader->nRef = 0;
	_pHeader->pPool = this;
	_pHeader->pNext = nullptr;

	MemoryBlock* pTemp1 = _pHeader;
	// initialize pool (each block)
	for (size_t i = 1; i < _nBlock; i++) {
		MemoryBlock* pTemp2 = (MemoryBlock*)(_pBuf + (i * each_buf));

		pTemp2->bPool = true;
		pTemp2->nID = i;
		pTemp2->nRef = 0;
		pTemp2->pPool = this;
		pTemp2->pNext = nullptr;
		// let the pointer of last block points to current block
		pTemp1->pNext = pTemp2;
		pTemp1 = pTemp2;
	}
}

// memory manager
MemoryMgr& MemoryMgr::getInstance() {
	return mgr;
}

// request memroy
void* MemoryMgr::allocMem(size_t nSize) {
	if (nSize <= MAX_MEMORY_SIZE) {
		// the requested memroy can be fit into memory pool
		return _szAlloc[nSize]->allocMem(nSize);
	}
	else {
		// request memory in heap
		MemoryBlock* pRet = (MemoryBlock*)malloc(nSize + sizeof(MemoryBlock));
		pRet->bPool = false;
		pRet->nID = -1;
		pRet->nRef = 1;
		pRet->pPool = nullptr;
		pRet->pNext = nullptr;
		xPrintf("allocMem: %llx, id=%d, size=%d \n", pRet, pRet->nID, nSize);
		return ((char*)pRet + sizeof(MemoryBlock));
	}
}

// free memory
void MemoryMgr::freeMem(void* pMem) {
	MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
	xPrintf("freeMem: %llx, id=%d \n", pBlock, pBlock->nID);
	if (pBlock->bPool) {
		// memroy in memory pool
		pBlock->pPool->freeMem(pMem);
	}
	else {
		if (--pBlock->nRef == 0) {
			// memory in heap
			free(pBlock);
		}
	}
}

void MemoryMgr::addRef(void* pMem) {
	// when a same memory is shared, increase ref count
	MemoryBlock* pBlock = (MemoryBlock*)((char*)pMem - sizeof(MemoryBlock));
	pBlock->nRef++;
}


// initialize mapping array
void MemoryMgr::init(int nBegin, int nEnd, MemoryPool* pMemP) {
	for (int i = nBegin; i <= nEnd; i++) {
		_szAlloc[i] = pMemP;
	}
}

// stop user trying to initialize manager
MemoryMgr::MemoryMgr() : _pool64{ 64, 100000 },
	_pool128{ 128, 100000 },
	_pool256{ 256, 100000 },
	_pool512{ 512, 100000 },
	_pool1024{ 1024, 100000 } {

	// when adjust max pool size, needs to redefine MAX_MEMORY_SIZE
	init(0, 64, &_pool64);
	init(65, 128, &_pool128);
	init(0, 128, &_pool128);
	init(129, 256, &_pool256);
	init(257, 512, &_pool512);
	init(512, 1024, &_pool1024);
};

MemoryMgr::~MemoryMgr() {};

MemoryMgr MemoryMgr::mgr{};