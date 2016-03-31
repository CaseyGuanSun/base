/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-14 19:32
** Email        : caseyguan@posteritytech.com
** Filename     : Condition.cpp
** Description  : Implement Condition class
** ******************************************************/

#include "Condition.h"


CCondition* CCondition::Create(CLock* pLock)
{
	CCondition* ptr = new CCondition(pLock);
	if(NULL == ptr)
	{
		return NULL;
	}
	return ptr;
}

CCondition::CCondition(CLock* pLock):
	m_pLock(pLock)
{
	if(!pLock)
	{
		assert(false);
	}
	pthread_cond_init(&m_cond, NULL);
}

CCondition::~CCondition()
{
	pthread_cond_destroy(&m_cond);
}

void CCondition::Wait()
{
	pthread_cond_wait(&m_cond, &(m_pLock->GetMutex()));
}

bool CCondition::WaitTime(uint32_t mWaitTime)
{
	const int MILLISECOND_PER_SECOND = 1000;
	const int MICROSECOND_PER_MILLISECOND = 1000;
	const int NANOSECOND_PER_SECOND = 1000000000;
	const int NANOSECOND_PER_MILLISECOND = 1000000;

	struct timespec ts;
	//struct timeval tv;
	//gettimeofday(&tv, NULL);
	clock_gettime(CLOCK_REALTIME, &ts);

	//ts.tv_sec = tv.tv_sec;
	//ts.tv_nsec = tv.tv_usec * MICROSECOND_PER_MILLISECOND;
	
	ts.tv_sec += mWaitTime/MILLISECOND_PER_SECOND;
	ts.tv_nsec += (mWaitTime - ((mWaitTime/MILLISECOND_PER_SECOND)*MILLISECOND_PER_SECOND))*NANOSECOND_PER_MILLISECOND;
	if(ts.tv_nsec >= NANOSECOND_PER_SECOND)
	{
		ts.tv_sec += ts.tv_nsec/NANOSECOND_PER_SECOND;
		ts.tv_nsec %= NANOSECOND_PER_SECOND;
	}
	const int res = pthread_cond_timedwait(&m_cond, &m_pLock->GetMutex(), &ts);
	return (res==ETIMEDOUT)?false:true;
}

void CCondition::Notify()
{
	pthread_cond_signal(&m_cond);
}

void CCondition::NotifyAll()
{
	pthread_cond_broadcast(&m_cond);
}
