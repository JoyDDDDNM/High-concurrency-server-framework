// client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// compile command in UNIX-like environment:
// g++ client.cpp -std=c++11 -pthread -o client

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "TcpClient.hpp"
#include "CELLTimestamp.hpp"
#include <thread>
#include <atomic>

// number of threads
const int tCount = 4;

// total number of clients 
const int cCount = 10000;

// clients array, it needs to be global so that each client in child thread can access it
EasyTcpClient* clients[cCount];

std::atomic_int sendCount = 0;
std::atomic_int readyCount = 0;

void childThread(int id) {
	if (!isRun) return;

	int begin = (id - 1) * (cCount / tCount);
	int end = (id) * (cCount / tCount);

	// initialize client
	for (int n = begin; n < end; n++) {
		clients[n] = new EasyTcpClient();
	}

	// connect server
	for (int n = begin; n < end; n++) {
		clients[n]->initSocket();
		clients[n]->connectServer("127.0.0.1", 4567);
	}

	// wait until all clients connect to server
	readyCount++;
	while (readyCount < tCount) {
		std::chrono::milliseconds t(10);
		std::this_thread::sleep_for(t);
	}

	Login login;
	strcpy(login.userName, "account");
	strcpy(login.password, "password");

	//CELLTimestamp timer;

	while (isRun) { 
		for (int n = begin; n < end; n++) {	
			// keep sending message to server 
			if (clients[n]->sendMessage(&login, sizeof(login)) != SOCKET_ERROR) {
				sendCount++;
			};

			// listen message from server
			//clients[n]->listenServer();
		}
	}

	for (int n = begin; n < end; n++) {
		clients[n]->closeSock();
		delete clients[n];
	}
}

int main()
{	
	// UI thread: create an thread for reading client input
	std::thread clientCmdThread(cmdThread);

	// Separates the thread of execution from the thread object, 
	// allowing execution to continue independently
	clientCmdThread.detach();

	// initialize threads
	for (int n = 0; n < tCount; n++) {
		std::thread t1(childThread, n+1);
		t1.detach();
	}

	CELLTimestamp timer;

	while (isRun) {
		auto t = timer.getElapsedSecond();
		
		if (t >= 1.0) {
			std::cout << tCount << " thread," << cCount <<" clients" << ",sending " << (int)(sendCount/t) << " messages in " << t << "s" << std::endl;
			sendCount = 0;
			timer.update();
		}

		Sleep(1);
	}

	//std::cout << count << std::endl;
	std::cout << "Client exit" << std::endl;
	std::cout << "Press Enter to exit" << std::endl;
	getchar();
	return 0;
}
