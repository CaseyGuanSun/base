/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-14 19:26
** Email        : caseyguan@posteritytech.com
** Filename     : Condition.h
** Description  : define Condition class 
** ******************************************************/

#ifndef _CONDITION_H_
#define _CONDITION_H_

#include "ostype.h"
#include "Lock.h"

class CCondition
{
public:
	static CCondition* Create(CLock* pLock);
	CCondition(CLock* pLock);
	~CCondition();
	void Wait();

	/*****
	 *nWaitTime is ms
	 *if receive signal then return true;
	 *else return false;
	 * ****/
	bool WaitTime(uint32_t nWaitTime);

	void Notify();
	void NotifyAll();
private:
	CLock* m_pLock;
	pthread_cond_t m_cond;
};

#endif


