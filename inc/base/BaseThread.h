/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-15 11:25
** Email        : caseyguan@posteritytech.com
** Filename     : BaseThread.h
** Description  : Define base thread class 
** ******************************************************/
#ifndef _BASE_THREAD_H_
#define _BASE_THREAD_H_

#include "ostype.h"
#include "Lock.h"
#include "Event.h"

#define ThreadObj void*

typedef bool (*ThreadRunFunction)(ThreadObj);

enum ThreadPriority
{
	PRIORITY_IDLE = -1,
	PRIORITY_NORMAL = 0,
	PRIORITY_ABOVE_NORMAL = 1,
	PRIORITY_HIGH = 2,
};


class CBaseThread
{
public:
	static CBaseThread* Create(ThreadRunFunction, ThreadObj, ThreadPriority prio, const char* thread_name);
	CBaseThread(ThreadRunFunction, ThreadObj, ThreadPriority prio, const char* thread_name);
	~CBaseThread();

	bool Start(unsigned int &id);
	bool Stop();
	bool SetAffinity(const int* processor_numbers, unsigned int amount_of_processors);

	void Run();
private:
	int Construct();
	uint32_t GetThreadId();
private:
	ThreadRunFunction  runFunction_;
	ThreadObj 	paramObj_;

	CLock	m_lock;	
	bool 	m_bAliveFlag;
	bool	m_bDeadFlag;
	CEvent* m_pEvent;

	ThreadPriority m_priority;
	pthread_t	m_threadId;
	pid_t       m_pid;
	char		m_threadName[128];
};

#endif
