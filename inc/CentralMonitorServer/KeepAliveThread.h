/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-17 18:14
** Email        : caseyguan@posteritytech.com
** Filename     : KeepAliveThread.h
** Description  : Define Keep Alive Thread Class
** ******************************************************/
#ifndef _KEEP_ALIVE_THREAD_H_
#define _KEEP_ALIVE_THREAD_H_
#include "ostype.h"
#include "BaseThread.h"
#include "ServerEndpointManager.h"
#include "ServerEndPoint.h"
#include "Debug.h"
#include "CentralMonitorServer.h"
#include "ProcessThread.h"

class CProcessThread;

class CKeepAliveThread
{
public:
	static CKeepAliveThread* Create(unsigned int intervalSec, unsigned int timeoutSec, CServerEndpintManager* pManager, CProcessThread* pProcessThread);
	static bool KeepAliveCb(void* param);
	CKeepAliveThread(unsigned int intervalSec, unsigned int timeoutSec, CServerEndpintManager* pManager, CProcessThread* pProcessThread);
	~CKeepAliveThread();
	bool Start();
	bool Stop();
private:
	bool Init();
	bool RunKeepAlive();
	int CheckServerEndpointState();
	int ProcessServerEndpoint(CServerEndpoint* endpoint);
	int SendKeepAliveMessage(CServerEndpoint* endpoint);	
	int CreateServerDownMsg(PROCESS_MSG* msg, CServerEndpoint* endpoint);
private:
	bool 	m_quitFlag;
	CBaseThread* pBaseThread;
	unsigned int m_intervalSecond;
	unsigned int m_timeoutSecond;
	CServerEndpintManager* m_endpointManager;
	CProcessThread* m_processThread;
};

#endif

