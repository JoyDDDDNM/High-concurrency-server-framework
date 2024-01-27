#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include "Client.hpp"

#include <thread>
#include <mutex>
#include <list>
#include <memory>

class CellTask {
	public:
		CellTask();

		virtual void doTask() = 0;

		virtual ~CellTask();

	private:

};

class CellTaskServer {
	public:
		using CellTaskPtr = std::shared_ptr<CellTask>;

		CellTaskServer();

		// launch server thread
		void start();

		void addTask(CellTaskPtr& task);
		
		~CellTaskServer();

	protected:
		void OnRun();

	private:
		// 
		std::list<CellTaskPtr> _tasks;

		std::list<CellTaskPtr> _tasksBuf;

		std::mutex _mutex;
		
		bool isRun;
};

// network message sending service
class CellSendMsgToClientTask : public CellTask {
public:
	CellSendMsgToClientTask(ClientPtr pClient, DataHeaderPtr& pHeader);

	virtual void doTask() override;

	virtual ~CellSendMsgToClientTask();

private:
	ClientPtr _pClient;
	DataHeaderPtr _pHeader;
};



#endif