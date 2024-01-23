#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include <thread>
#include <mutex>
#include <list>
#include <functional>
#include <memory>

class CellTask {
	public:
		CellTask() {
		
		};

		virtual void doTask() {
					
		}

		virtual ~CellTask() {};
	private:

};

class CellTaskServer {
	public:
		using CellTaskPtr = std::shared_ptr<CellTask>;

		CellTaskServer() :_tasks{}, _tasksBuf{}, _mutex{}, isRun{ true } {}

		// launch server thread
		void start() {
			isRun = true;
			std::thread t(std::mem_fn(&CellTaskServer::OnRun), this);
			t.detach();
		}

		void addTask(CellTaskPtr& task) {
			if (!isRun) {
				return;
			}

			std::lock_guard<std::mutex> lock(_mutex);
			_tasksBuf.push_back(task);
		}
		
		~CellTaskServer() {}

	protected:
		void OnRun() {
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

	private:
		// 
		std::list<CellTaskPtr> _tasks;

		std::list<CellTaskPtr> _tasksBuf;

		std::mutex _mutex;
		
		bool isRun;
};


#endif