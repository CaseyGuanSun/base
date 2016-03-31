/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-15 11:26
** Email        : caseyguan@posteritytech.com
** Filename     : BaseThread.cpp
** Description  : Implement base thread class 
** ******************************************************/
#include <sys/syscall.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include "BaseThread.h"
#include "Sleep.h"

CBaseThread* CBaseThread::Create(ThreadRunFunction func, ThreadObj obj,
                                   ThreadPriority prio,
                                   const char* thread_name) {
  CBaseThread* ptr = new CBaseThread(func, obj, prio, thread_name);
  if (!ptr) {
    return NULL;
  }
  const int error = ptr->Construct();
  if (error) {
    delete ptr;
    return NULL;
  }
  return ptr;
}


extern "C"
{
  static void* StartThread(void* lp_parameter) {
    static_cast<CBaseThread*>(lp_parameter)->Run();
    return 0;
  }
}

CBaseThread::CBaseThread(ThreadRunFunction func, ThreadObj obj, ThreadPriority prio, const char* thread_name):
		runFunction_(func)
	   ,paramObj_(obj)
	   ,m_priority(prio)
	   ,m_bAliveFlag(false)
	   ,m_bDeadFlag(true)
	   ,m_pEvent(CEvent::Create())
{
	memset(m_threadName, 0x00, sizeof(m_threadName));
	if(thread_name)	
	{
		strncpy(m_threadName, thread_name, sizeof(m_threadName)-1);
		m_threadName[sizeof(m_threadName)-1] = '\0';
	}
	
}

CBaseThread::~CBaseThread()
{
	if(m_pEvent)
	{
		delete m_pEvent;
	}
}

int CBaseThread::Construct()
{
	if(m_pEvent == NULL)
	{
		return -1;
	}
	if(NULL == runFunction_)
	{
		return -1;
	}
		
	return 0;
}

bool CBaseThread::Start(unsigned int &thread_id)
{
	int result = 0;
	pthread_attr_t attr;

	result = pthread_attr_init(&attr);
	if(0 != result)
	{
		return false;
	}
	result = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	result |= pthread_attr_setstacksize(&attr, 1024*1024);
	if(0 != result)
	{
		return false;
	}
	m_pEvent->Reset();
	result |= pthread_create(&m_threadId, &attr, &StartThread, this);
	if(0 != result)
	{
		return false;
	}

	{
		CriticalSectionScoped cs(&m_lock);
		m_bDeadFlag = false;	
	}

	if(EventTypeSignal != m_pEvent->Wait(EVENT_10_SECOND))
	{
		return true;
	}
	return true;
}

bool CBaseThread::Stop()
{
	bool dead = false;
	int i=0;

	{
		CriticalSectionScoped cs(&m_lock);
		m_bAliveFlag = false;
		dead = m_bDeadFlag;
	}

	for (int i = 0; i < 1000 && !dead; ++i) {
	   SleepMs(10);
	   {
		 CriticalSectionScoped cs(&m_lock);
		 dead = m_bDeadFlag;
	   }
	}
	if (dead) {
	   return true;
	} else {
	   return false;
	}
}

bool CBaseThread::SetAffinity(const int* processor_numbers, unsigned int amount_of_processors)
{
	if (!processor_numbers || (amount_of_processors == 0)) {
    	return false;
  	}
  	cpu_set_t mask;
  	CPU_ZERO(&mask);

	for (unsigned int processor = 0;
    	processor < amount_of_processors;
    	++processor) {
    	CPU_SET(processor_numbers[processor], &mask);
  	}

  	// "Normal" Linux.
  	const int result = sched_setaffinity(m_pid,
                                       sizeof(mask),
                                       &mask);

  	if (result != 0) {
    	return false;
  	}
  	return true;
}

void CBaseThread::Run() {
	{
		CriticalSectionScoped cs(&m_lock);
    	m_bAliveFlag = true;
  	}

  	m_pid = GetThreadId();

  	// The event the Start() is waiting for.
  	m_pEvent->Set();

  	if (strlen(m_threadName) > 0) {
    	prctl(PR_SET_NAME, (unsigned long)m_threadName, 0, 0, 0);
  	} else {
		//TODO:print log
  	}
  	bool alive = true;
  	bool run = true;
  	while (alive) {
    	run = runFunction_(paramObj_);
    	CriticalSectionScoped cs(&m_lock);
    	if (!run) {
      		m_bAliveFlag= false;
    	}
    	alive = m_bAliveFlag;
  	}
  	{
  		CriticalSectionScoped cs(&m_lock);
    	m_bDeadFlag= true;
  	}
}


uint32_t CBaseThread::GetThreadId() {
 // return static_cast<uint32_t>(syscall(__NR_gettid));
//	return syscall(__NR_gettid);
	//TODO:
	return 0;
}


