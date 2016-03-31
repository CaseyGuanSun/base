/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-14 19:25
** Email        : caseyguan@posteritytech.com
** Filename     : Lock.cpp
** Description  : Implement the Lock class
** ******************************************************/

#include "Lock.h"

CLock::CLock()
{
	pthread_mutex_init(&m_mutex, NULL);
}

CLock::~CLock()
{
	pthread_mutex_destroy(&m_mutex);
}

void CLock::Lock()
{
	pthread_mutex_lock(&m_mutex);
}

void CLock::Unlock()
{
	pthread_mutex_unlock(&m_mutex);
}
