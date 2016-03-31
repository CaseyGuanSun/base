/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-14 18:02
** Email        : caseyguan@posteritytech.com
** Filename     : Lock.h
** Description  : define lock class
** ******************************************************/

#ifndef _LOCK_H_
#define _LOCK_H_

#include "ostype.h"


class CLock
{
public:
	CLock();
	virtual ~CLock();
    void Lock();
	void Unlock();
	pthread_mutex_t& GetMutex(){return m_mutex;}
private:
	pthread_mutex_t m_mutex;
};

class CriticalSectionScoped
{
public:
	CriticalSectionScoped(CLock* pLock):
		m_pLock(pLock)
	{
		m_pLock->Lock();
	}
	~CriticalSectionScoped()
	{
		m_pLock->Unlock();
	}
private:
	CLock *m_pLock;
};

#endif



