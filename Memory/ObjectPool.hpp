#ifndef _OBJECT_POOL_HPP
#define _OBJECT_POOL_HPP

#include <stdlib.h>
#include <assert.h>
#include <mutex>
#include <iostream>
#include "Alloc.hpp"

// DEBUG print
#ifdef _DEBUG
	#ifndef xPrintf
		#include <stdio.h>
		#define xPrintf(...) printf( __VA_ARGS__ )
	#endif // !xPrintf
#else
	#ifndef xPrintf
		#define xPrintf(...)
	#endif // !xPrintf
#endif

// poolSize: number of block in object pool
template<typename T, size_t poolSize>
class ObjectPool {
	public:
		ObjectPool() :_pBuf{ nullptr }, _pHeader{ nullptr } {
			initPool();
		};

		~ObjectPool() {
			if (_pBuf) delete[] _pBuf;
		};

		// request object
		void* allocMem(size_t nSize) {
			std::lock_guard<std::mutex> lock(_mutex);

			if (!_pBuf) initPool();

			NodeHeader* pRet{ nullptr };

			if (_pHeader == nullptr) {
				pRet = (NodeHeader*)new char[sizeof(T) + sizeof(NodeHeader)];
				pRet->bPool = false;
				pRet->nID = -1;
				pRet->nRef = 1;
				pRet->pNext = nullptr;
			}
			else {
				pRet = _pHeader;
				_pHeader = _pHeader->pNext;
				assert(pRet->nRef == 0);
				pRet->nRef = 1;
			}

			xPrintf("allocMem: %llx, id=%d, size=%d \n", pRet, pRet->nID, nSize);

			return ((char*)pRet + sizeof(NodeHeader));
		}

		// release object
		void freeMem(void* pMem) {

			// pointer to the header of each block of object pool 
			NodeHeader* pBlock = (NodeHeader*)((char*)pMem - sizeof(NodeHeader));

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
				
				delete pBlock;
			}
		}


		class NodeHeader {
			public:
				// pointer to next
				NodeHeader* pNext;

				// id number 
				int nID;

				// number of times to get referenced
				short nRef;

				// check if node is in pool
				bool bPool;

			private:
				// padding
				char c1;
		};

	private:
		// initialize pool
		void initPool() {
			assert(_pBuf == nullptr);
			// request memory from memory buffer
			size_t realSzie = sizeof(T) + sizeof(NodeHeader);
			size_t n = poolSize * realSzie;

			// determine what size of memory pool we use 
			_pBuf = new char[n];

			// initialize object buffer
			_pHeader = (NodeHeader*)_pBuf;
			_pHeader->bPool = true;
			_pHeader->nID = 0;
			_pHeader->nRef = 0;
			_pHeader->pNext = nullptr;

			NodeHeader* pTemp1 = _pHeader;
			 
			for (size_t n = 1; n < poolSize; n++) {
				NodeHeader* pTemp2 = (NodeHeader*)(_pBuf + (n * realSzie));
				pTemp2->bPool = true;
				pTemp2->nID = n;
				pTemp2->nRef = 0;
				pTemp2->pNext = nullptr;
				pTemp1->pNext = pTemp2;
				pTemp1 = pTemp2;
			}
		}

		NodeHeader* _pHeader;

		// buffer of object pool
		char* _pBuf;

		std::mutex _mutex;
		
};

// set default pool size to 10
template<typename T, size_t poolSize = 10>
class ObjectPoolBase {
	// similarly with memory pool manager, we use ObjectPoolBase 
	// to request object pool
	public:
		ObjectPoolBase() {}

		// overload new to avoid using memory from memory pool
		void* operator new(size_t nSize) {
			// nSize is the size of object (not the parameter we pass in), which is calculated during running time
			return getInstance().allocMem(nSize);
		}

		void operator delete(void* p) {
			getInstance().freeMem(p);
		}

		// variadic template
		// a wrapper to invoke new to request memory for our object, instead of using the 
		// overloaded new directly
		template<typename ...Args>
		static T* createObject(Args...args) {
			// using the overload new and args to call the corresponding constructor
			return new T(args...);
		}

		static void releaseObject(T* p) {
			delete p;
		}

		~ObjectPoolBase() {}

	private:
		static ObjectPool<T, poolSize>& getInstance() {
			static ObjectPool<T, poolSize> pool;
			return pool;
		}
};

#endif