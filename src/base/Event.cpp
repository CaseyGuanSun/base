#include "Event.h"
extern "C"{
#include "ipcsock.h"
}

CEvent* CEvent::Create()
{
	CEvent* ptr = new CEvent();
	if(NULL == ptr)
	{
		return NULL;
	}
	int result  = ptr->Construct();
	if(result < 0)
	{
		delete ptr;
		return NULL;
	}
	return ptr;
}

CEvent::CEvent():
	m_state(EventStateDown)
{
}

CEvent::~CEvent()
{
	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_cond);
}

int CEvent::Construct()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	int result  = pthread_mutex_init(&m_mutex, &attr);
	if(result != 0)
	{
		return -1;
	}

	pthread_condattr_t cond_attr;
	pthread_condattr_init(&cond_attr);
	result = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	if(result != 0)
	{
		return -1;
	}
	result =  pthread_cond_init(&m_cond, &cond_attr);
	if(result != 0)
	{
		return -1;
	}
	result = pthread_condattr_destroy(&cond_attr);
	if(result != 0)
	{
		return -1;
	}
	return 0;
}

bool CEvent::Set()
{
	if(0 != pthread_mutex_lock(&m_mutex))
	{
		return false;
	}
	m_state = EventStateUp;
	pthread_cond_broadcast(&m_cond);
	pthread_mutex_unlock(&m_mutex);
	return true;
}

bool CEvent::Reset()
{
	if(0 != pthread_mutex_lock(&m_mutex))
	{
		return false;
	}
	m_state = EventStateDown;
	pthread_mutex_unlock(&m_mutex);
	return true;
}

EventType CEvent::Wait(unsigned long mWaitTime)
{
	int ret = 0;

	if(0 != pthread_mutex_lock(&m_mutex))
	{
		return EventTypeError; 
	}

	if(EventStateDown == m_state)	
	{
		if(EVENT_INFINITE != mWaitTime)
		{
			 const int MILLISECOND_PER_SECOND = 1000;
		     const int MICROSECOND_PER_MILLISECOND = 1000;
		     const int NANOSECOND_PER_SECOND = 1000000000;
		     const int NANOSECOND_PER_MILLISECOND = 1000000;
		 
		     struct timespec ts;
		     struct timeval tv;
		     gettimeofday(&tv, NULL);
		 
		     ts.tv_sec = tv.tv_sec;
		     ts.tv_nsec = tv.tv_usec * MICROSECOND_PER_MILLISECOND;
		
		     ts.tv_sec = mWaitTime/MILLISECOND_PER_SECOND;
		     ts.tv_nsec = (mWaitTime - ((mWaitTime/MILLISECOND_PER_SECOND)*MILLISECOND_PER_SECOND))*NANOSECOND_PER_MILLISECOND;
		     if(ts.tv_nsec >= NANOSECOND_PER_SECOND)
	     	 {
				 ts.tv_sec += ts.tv_nsec/NANOSECOND_PER_SECOND;
			     ts.tv_nsec %= ts.tv_nsec%NANOSECOND_PER_SECOND;
			 }
		     ret = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
		}else
		{
			ret = pthread_cond_wait(&m_cond, &m_mutex);
		}

	}
	m_state = EventStateDown;
	pthread_mutex_unlock(&m_mutex);

	switch(ret)
	{
	case 0:
		return EventTypeSignal;
	case ETIMEDOUT:
		return EventTypeTimeout;
	default:
		return EventTypeError;
	}
}

