#include <stdlib.h>
#include <iostream>
#include <thread>
#include <memory>
#include "Alloc.hpp"
#include "CELLTimestamp.hpp"

using namespace std;
 
// number of threads
const int tCount = 8;

// number of clients
const int mCount = 100000;

// number of clients in each thread
const int nCount = mCount / tCount;

void workFun(int index) {
	char* data[nCount];
	for (size_t i = 0; i < nCount; i++) {
		data[i] = new char[(rand() % 128) + 1];
	}
	for (size_t i = 0; i < nCount; i++) {
		delete[] data[i];
	}
	
}

int main() {
	// thread t[tCount];
	// CELLTimestamp tTime;
	// 
	// for (int n = 0; n < tCount; n++) {
	// 	t[n] = thread(workFun, n);
	// 	t[n].join();
	// }
	// 
	// cout << tTime.getElapsedTimeInMilliSec() << endl;
	// cout << "Hello,main thread." << endl;
	// getchar();
	
	std::shared_ptr<int> ptr = std::make_shared<int>();
	
	std::cout << sizeof(ptr) << std::endl;

	return 0;
}