/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-15 20:33
** Email        : caseyguan@posteritytech.com
** Filename     : Sleep.h
** Description  : Define Sleep Funcion
**                This function sleep for a specified number of milliseconds.It maybe return early if the thread is woken by other event,such as the delivery 
				  a signal on Unix
** ******************************************************/
#ifndef _SLEEP_H_
#define _SLEEP_H_

#include "ostype.h"

void SleepMs(int msecs);

#endif
