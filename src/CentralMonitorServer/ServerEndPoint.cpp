/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 12:12
** Email        : caseyguan@posteritytech.com
** Filename     : ServerEndPoint.cpp
** Description  : Define Server Endpoint Class
** ******************************************************/
#include "ServerEndPoint.h"

CServerEndpoint::CServerEndpoint():
	m_serverState(SERVER_STATE_INIT)
   ,m_keepAliveCount(0)
{

}

CServerEndpoint::~CServerEndpoint()
{
}

CServerEndpoint& CServerEndpoint::operator=(const CServerEndpoint& endpoint)
{
	this->m_sockFd = endpoint.m_sockFd;
	this->m_strConnectIp = endpoint.m_strConnectIp;
	this->m_connectPort = endpoint.m_connectPort;
	this->m_strPublicIp = endpoint.m_strPublicIp;
	this->m_strHostResourceId = endpoint.m_strHostResourceId;
	this->m_strHostRegion = endpoint.m_strHostRegion;
	this->m_strEIPResourceId = endpoint.m_strEIPResourceId;
	this->m_strEIPRegion = endpoint.m_strEIPRegion;
	this->m_strDBIp = endpoint.m_strDBIp;
	this->m_strDBName = endpoint.m_strDBName;
	this->m_serverRole = endpoint.m_serverRole;
	this->m_serverState = endpoint.m_serverState;
	this->m_keepAliveCount = endpoint.m_keepAliveCount;
	this->m_updateTimer = endpoint.m_updateTimer;

	return *this;
}

char* CServerEndpoint::GetServerEndpointState(char* buffer)
{
	buffer[0] = '\0';
	switch(m_serverState)
	{
	case SERVER_STATE_INIT:
		 sprintf(buffer, "INIT"); 
		 break;
	case SERVER_STATE_DOWN:
		 sprintf(buffer, "DOWN");
		 break;
	case SERVER_STATE_DISCONNECTED:
		 sprintf(buffer, "DISCONNECTED");
		 break;
	case SERVER_STATE_DOWNING:
		 sprintf(buffer, "DOWNING");
		 break;
	case SERVER_STATE_RUNNING:
		 sprintf(buffer, "RUNNING");
		 break;
	case SERVER_STATE_DISCONNECTING:
		 sprintf(buffer, "DISCONNECTING");
		 break;
	}
	return buffer;
}

char* CServerEndpoint::GetServerEndpointRole(char* buffer)
{
	buffer[0] = '\0';
	switch(m_serverRole)
	{
	case SERVER_ROLE_WORK:
		sprintf(buffer, "Master");
		break;
	case SERVER_ROLE_STANDBY:
		sprintf(buffer, "Standby");
		break;
	}

	return buffer;
}





