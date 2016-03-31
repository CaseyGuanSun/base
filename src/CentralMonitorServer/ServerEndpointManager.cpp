/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 17:03
** Email        : caseyguan@posteritytech.com
** Filename     : ServerEndpointManager.cpp
** Description  : Implement Server Endpoint Manager Class
** ******************************************************/
#include "ServerEndpointManager.h"
extern "C"
{
#include "ipcsock.h"
}
#include "CentralMonitorServer.h"

#define TAG "ServerEndpointManager"

CServerEndpintManager::CServerEndpintManager()
{
}

CServerEndpintManager::~CServerEndpintManager()
{
	m_serverEndpontArray.clear();
}

bool CServerEndpintManager::AddServerEndpoint(int sockFd, string& connectIp, uint16_t port)
{
	CServerEndpoint* pEndpoint = NULL;
	pEndpoint = GetServerEndpointByIp(connectIp);
	if(pEndpoint == NULL)
	{
		//this is new end point
		CServerEndpoint endpoint;
		CriticalSectionScoped cs(&m_lock);
		endpoint.m_sockFd = sockFd;
		endpoint.m_strConnectIp = connectIp;
		endpoint.m_connectPort = port;
		endpoint.m_serverState = SERVER_STATE_INIT;
		endpoint.m_downCount = 0;
		endpoint.m_updateTimer = getCurrentTime_t();
		m_serverEndpontArray.push_back(endpoint);
	}else
	{
		//This is an ole entray
		pEndpoint->m_sockFd = sockFd;
		pEndpoint->m_strConnectIp = connectIp;
		pEndpoint->m_connectPort = port;
		pEndpoint->m_serverState = SERVER_STATE_INIT;
		pEndpoint->m_downCount = 0;
		pEndpoint->m_updateTimer = getCurrentTime_t();
	}
	return true;
}


bool CServerEndpintManager::UpdateServerEndpointRegister(int sockFd, const char* hostRegion, const char* publicIp, 
		const char* hostResourceId, const char* eipResourceId, const char* eipRegion, const char* dbIp, const char* dbName, ServerRole serverRole, ServerState state)
{
	CriticalSectionScoped cs(&m_lock);
	int i=0;
	for(i=0; i<m_serverEndpontArray.size(); i++)
	{
		if(m_serverEndpontArray[i].m_sockFd == sockFd)
		{
			m_serverEndpontArray[i].m_strHostRegion = hostRegion;
			m_serverEndpontArray[i].m_strPublicIp = publicIp;
			m_serverEndpontArray[i].m_strHostResourceId = hostResourceId;
			m_serverEndpontArray[i].m_strEIPResourceId = eipResourceId;
			m_serverEndpontArray[i].m_strEIPRegion = eipRegion;
			m_serverEndpontArray[i].m_strDBIp = dbIp;
			m_serverEndpontArray[i].m_strDBName = dbName;
			m_serverEndpontArray[i].m_serverRole = serverRole;
			m_serverEndpontArray[i].m_serverState = state;
			return true;
		}
	}
	return false;
}

CServerEndpoint* CServerEndpintManager::GetServerEndpoint(int index)
{
	CriticalSectionScoped cs(&m_lock);
	if(0<=index && index < m_serverEndpontArray.size())
	{
		return &m_serverEndpontArray[index];
	}
	return NULL;
}

CServerEndpoint* CServerEndpintManager::GetServerEndpointByIp(string &connectIp)
{
	CriticalSectionScoped cs(&m_lock);
	int i=0;
	for(i=0; i<m_serverEndpontArray.size(); i++)
	{
		if(connectIp == m_serverEndpontArray[i].m_strConnectIp)
		{
			return &m_serverEndpontArray[i];
		}
	}
	return NULL;
}

CServerEndpoint* CServerEndpintManager::GetServerEndpointBySockFd(int sockFd)
{
	CriticalSectionScoped cs(&m_lock);
	int i=0;
	for(i=0; i<m_serverEndpontArray.size(); i++)
	{
		if(sockFd == m_serverEndpontArray[i].m_sockFd)
		{
			return &m_serverEndpontArray[i];
		}
	}
	return NULL;
}

bool CServerEndpintManager::DeleteServerEndpoint(int sockFd)
{
	CriticalSectionScoped cs(&m_lock);
	int i=0;
	for(i=0; i<m_serverEndpontArray.size(); i++)
	{
		if(sockFd == m_serverEndpontArray[i].m_sockFd)
		{
			m_serverEndpontArray.erase(m_serverEndpontArray.begin()+i);
			return true;
		}
	}
	return false;
}


int  CServerEndpintManager::GetServerEndpointCount()
{
	CriticalSectionScoped cs(&m_lock);
	return m_serverEndpontArray.size();
}

bool CServerEndpintManager::CheckServerEndpoint(int sockFd)
{
	int i;

	CriticalSectionScoped cs(&m_lock);
	for(i=0; i< m_serverEndpontArray.size(); i++)
	{
		if(m_serverEndpontArray[i].m_sockFd == sockFd)
		{
			return true;
		}
	}
	return false;
}

bool CServerEndpintManager::UpdateServerEndpointState(int sockFd, ServerState state)
{
	bool res = false;
	char stateBuf[128];
	char roleBuf[128];
	CServerEndpoint* pEndpoint = GetServerEndpointBySockFd(sockFd);
	if(pEndpoint == NULL)
	{
		return false;
	}

	memset(stateBuf, 0x00, sizeof(stateBuf));
	memset(roleBuf, 0x00, sizeof(roleBuf));
	switch(state)
	{
	case SERVER_STATE_RUNNING:
		pEndpoint->m_downCount = 0;
		pEndpoint->m_updateTimer = getCurrentTime_t();
		pEndpoint->m_serverState = SERVER_STATE_RUNNING;
		res = true;
		break;	
	case SERVER_STATE_DOWN:
		//has problem
		g_debug->DebugMsg(LOG_LEVEL_WARNING, TAG, "The Server Endpoint is down, agent IP:%s serverState:%s, server Role:%s, m_downCount:%d", pEndpoint->m_strConnectIp.c_str(), pEndpoint->GetServerEndpointState(stateBuf), pEndpoint->GetServerEndpointRole(roleBuf), pEndpoint->m_downCount);
		pEndpoint->m_updateTimer = getCurrentTime_t();
		if(pEndpoint->m_serverRole == SERVER_ROLE_WORK)
		{
			if(pEndpoint->m_serverState == SERVER_STATE_RUNNING)
			{
				pEndpoint->m_serverState = SERVER_STATE_DOWNING;
				pEndpoint->m_downCount = 1;
			}else if(pEndpoint->m_serverState == SERVER_STATE_DOWN)
			{
				pEndpoint->m_downCount = 0;
			}
			if(pEndpoint->m_serverState == SERVER_STATE_DOWNING)
			{
				pEndpoint->m_downCount++;
			}
		}else if(pEndpoint->m_serverRole == SERVER_ROLE_STANDBY)
		{
			pEndpoint->m_serverState = SERVER_STATE_DOWN;
		}
		res = true;
		break;
	case SERVER_STATE_DISCONNECTED:
		g_debug->DebugMsg(LOG_LEVEL_WARNING, TAG, "The Server Endpoint is disconnected, agent IP:%s serverState:%s, server Role:%s", pEndpoint->m_strConnectIp.c_str(), pEndpoint->GetServerEndpointState(stateBuf), pEndpoint->GetServerEndpointRole(roleBuf));
		if(pEndpoint->m_serverState == SERVER_STATE_INIT)
		{
			DeleteServerEndpoint(sockFd);
		}else if(pEndpoint->m_serverState == SERVER_STATE_DOWNING)
		{
			pEndpoint->m_serverState = SERVER_STATE_DISCONNECTING;
			pEndpoint->m_updateTimer = getCurrentTime_t();
		}else if(pEndpoint->m_serverState == SERVER_STATE_DOWN)
		{
			pEndpoint->m_serverState = SERVER_STATE_DISCONNECTED;
			pEndpoint->m_updateTimer = getCurrentTime_t();
		}else if(pEndpoint->m_serverState == SERVER_STATE_RUNNING)
		{
			pEndpoint->m_serverState = SERVER_STATE_DISCONNECTING;
			pEndpoint->m_updateTimer = getCurrentTime_t();
		}
		res = true;
		break;
	}
	return res;
}

CServerEndpoint* CServerEndpintManager::GetNewStandbyServer()
{
	int i;
	
	CriticalSectionScoped cs(&m_lock);
	for(i=0; i < m_serverEndpontArray.size(); i++)
	{
		if(m_serverEndpontArray[i].m_serverRole == SERVER_ROLE_STANDBY &&
			m_serverEndpontArray[i].m_serverState != SERVER_STATE_DISCONNECTED &&
			m_serverEndpontArray[i].m_serverState != SERVER_STATE_DISCONNECTING)
		{
			return &m_serverEndpontArray[i];
		}
	}

	return NULL;
}

void CServerEndpintManager::PrintAllServerEndpoint()
{
	int i=0;
	char roleBuf[128];
	char stateBuf[128];

	CServerEndpoint* pEndpoint = NULL;

	CriticalSectionScoped cs(&m_lock);
	memset(roleBuf, 0x00 ,sizeof(roleBuf));
	memset(stateBuf, 0x00, sizeof(stateBuf));
	for(i=0; i<m_serverEndpontArray.size(); i++)
	{
		pEndpoint = &m_serverEndpontArray[i];
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "id:%d, sockFd:%d, connectIp:%s, connectPort:%d, hostResourceId:%s,"\
		" HostRegion:%s, IpResourceId:%s, IpRegion:%s, publicIp:%s, DBIp:%s, DBName:%s, Role:%s, State:%s", i, 
		pEndpoint->m_sockFd, pEndpoint->m_strConnectIp.c_str(), pEndpoint->m_connectPort, 
		pEndpoint->m_strHostResourceId.c_str(), pEndpoint->m_strHostRegion.c_str(),pEndpoint->m_strEIPResourceId.c_str(), 
		pEndpoint->m_strEIPRegion.c_str(),pEndpoint->m_strPublicIp.c_str(), pEndpoint->m_strDBIp.c_str(), 
		pEndpoint->m_strDBName.c_str(), pEndpoint->GetServerEndpointRole(roleBuf), pEndpoint->GetServerEndpointState(stateBuf));
	}
}




