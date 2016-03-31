/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 17:03
** Email        : caseyguan@posteritytech.com
** Filename     : ServerEndpointManager.h
** Description  : Define Server Endpoint Manager Class
** ******************************************************/
#ifndef _SERVER_ENDPOINT_MANAGER_H_
#define _SERVER_ENDPOINT_MANAGER_H_
#include <vector>
#include "ostype.h"
#include "ServerEndPoint.h"
#include "Lock.h"

using namespace std;

class CServerEndpintManager
{
public:
	CServerEndpintManager();
	~CServerEndpintManager();

	bool AddServerEndpoint(int sockFd, string& connectIp, uint16_t port);
	bool DeleteServerEndpoint(int sockFd);
	CServerEndpoint* GetServerEndpointByIp(string &connectIp);
	bool UpdateServerEndpointRegister(int sockFd, const char* hostRegion, const char* publicIp, const char* hostResourceId, const char* eipResourceId, const char* eipRegion, const char* dbIp, const char* dbName, ServerRole serverRole, ServerState state);
	bool UpdateServerEndpointState(int sockFd, ServerState state);
	bool DeleteServerEndpoint();
	CServerEndpoint* GetServerEndpointBySockFd(int sockFd);
	CServerEndpoint* GetServerEndpoint(int index);
	int  GetServerEndpointCount();
	bool CheckServerEndpoint(int sockFd);
	CServerEndpoint* GetNewStandbyServer();
	void PrintAllServerEndpoint();
private:
	CLock	m_lock;
	vector<CServerEndpoint> m_serverEndpontArray;
};

#endif

