/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 16:49
** Email        : caseyguan@posteritytech.com
** Filename     : ProcessThread.h
** Description  : Define the Process Thread Class
** ******************************************************/
#ifndef _PROCESS_THREAD_H_
#define _PROCESS_THREAD_H_
#include <vector>
#include <map>
#include "ostype.h"
#include "BaseThread.h"
#include "ServerEndPoint.h"
#include "ServerEndpointManager.h"
#include "general.h"
#include "Lock.h"
#include "Condition.h"
#include "CentralMonitorServer.h"
#include "ApiInfo.h"
#include "HttpClient.h"
#include "RequestParam.h"

using namespace std;

#define MAX_PROCESS_MSG	100

#define RESOURCE_TYPE  "uhost"

class CProcessThread
{
public:
	static CProcessThread* Create(CServerEndpintManager* pManager, ApiInfo* apiInfo);
	static bool ProcessCbFunction(ThreadObj param);
	CProcessThread(CServerEndpintManager* pManager, ApiInfo* apiInfo);
	~CProcessThread();
	bool Start();
	bool Stop();
	int putProcessMessage(PROCESS_MSG *msg);
	int getProcessMessage(PROCESS_MSG* msg);
private:
	bool Init();
	bool ProcessFunction();
	bool ProcessMsg(PROCESS_MSG* msg);

	bool ProcessRegisterMsg(PROCESS_MSG* msg);
	bool ProcessKeepAliveAckMsg(PROCESS_MSG* msg);
	bool ProcessStartWorkAckMsg(PROCESS_MSG* msg);
	bool ProcessSocketDownMsg(PROCESS_MSG* msg);
	bool ProcessServerDownMsg(PROCESS_MSG* msg);
	bool SendRegisterAckMsg(int sockFd, int status);
	bool BindEIP(string region, string eipId, string resourceType, string resouceId);
	bool UnbindEIP(string region, string eipId, string resourceType, string resouceId);
	bool SendStartWorkMsg(int sockFd, string &publicIp, string &eipResourceId, string &eipRegion, string &dbIp, string &dbName);
	bool ChangeFloatIp(CServerEndpoint* oldEndpoint);
public:
	bool GetUHostInfo(string region, string hostId);
private:
	bool SendPostRequest(map<string,RequestParam> &paramMap);
private:
	bool 	m_bQuitFlag;
	CServerEndpintManager* m_pServerEndpointManager;
	CBaseThread* m_pThread;
	unsigned int 	m_threadId;
	ApiInfo* 	m_apiInfo;

	CLock		m_lock;
	CCondition*	m_pCondition;
	vector<PROCESS_MSG> m_msgQueue;
	unsigned int m_start;
	unsigned int m_end;
	unsigned int m_count;

	CHttpClient httpClient;
};

#endif


