/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-15 12:19
** Email        : caseyguan@posteritytech.com
** Filename     : Event.h
** Description  : Define Event class 
** ******************************************************/
#ifndef _EVENT_H_
#define _EVENT_H_

#include "ostype.h"

#define EVENT_10_SECOND  10000
#define EVENT_INFINITE	0xffffffff

enum EventType
{
	EventTypeSignal = 0,
	EventTypeTimeout,
	EventTypeError
};

enum EventState
{
	EventStateDown = 0,
	EventStateUp = 1
};

class CEvent
{
public:
	static CEvent* Create();
	CEvent();
	~CEvent();
	bool Set();
	bool Reset();
	EventType Wait(unsigned long mWaitTime);
private:
	int Construct();

private:
	pthread_mutex_t m_mutex;
	pthread_cond_t  m_cond;
	EventState m_state;
};


#endif



