#include "CELLTask.hpp"

#include <thread>
#include <mutex>
#include <functional>

CellTask::CellTask() = default;

CellTask::~CellTask() = default;

CellTaskServer::CellTaskServer() :_tasks{}, _tasksBuf{}, _mutex{}, isRun{ true } {};

CellTaskServer::~CellTaskServer() = default;

void CellTaskServer::start() {
	isRun = true;
	std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
	t.detach();
}

void CellTaskServer::addTask(CellTaskPtr& task) {
	if (!isRun) {
		return;
	}

	std::lock_guard<std::mutex> lock(_mutex);
	_tasksBuf.push_back(task);
}

void CellTaskServer::OnRun() {
	if (!isRun) {
		return;
	}

	while (true) {
		// move tasks from buffer to queue
		if (!_tasksBuf.empty()) {
			std::lock_guard<std::mutex> lock(_mutex);
			for (auto task : _tasksBuf) {
				_tasks.push_back(task);
			}
			_tasksBuf.clear();
		}

		if (_tasks.empty()) {
			std::chrono::milliseconds t(1);
			std::this_thread::sleep_for(t);
			continue;
		}

		for (auto task : _tasks) {
			task->doTask();
		}

		_tasks.clear();
	}
}

CellSendMsgToClientTask::CellSendMsgToClientTask(ClientPtr pClient, DataHeaderPtr& pHeader) : _pClient{ pClient }, _pHeader{ pHeader } {}

void CellSendMsgToClientTask::doTask() {
	_pClient->sendMessage(_pHeader);
}

CellSendMsgToClientTask::~CellSendMsgToClientTask() = default;