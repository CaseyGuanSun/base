/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 18:43
** Email        : caseyguan@posteritytech.com
** Filename     : general.h
** Description  : Define general data type
** ******************************************************/
#ifndef _GENERAL_H_
#define _GENERAL_H_
extern "C"
{
#include "ipcsock.h"
}

enum PROCESS_MSG_TYPE
{
	MSG_TYPE_AGENT = 0,
	MSG_TYPE_HOST
};

typedef struct PROCESS_MSG
{
	int 				sockFd;
	PROCESS_MSG_TYPE 	msgType;
	char 				msgBuf[MAXTCPPACKETLEN+1];
	char				msgLen;
}PROCESS_MSG;

#endif

