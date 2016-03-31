/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 12:12
** Email        : caseyguan@posteritytech.com
** Filename     : ServerEndPoint.h
** Description  : Define Server Endpint Class
** ******************************************************/
#ifndef _SERVER_ENDPOINT_H_
#define _SERVER_ENDPOINT_H_
#include <string>
#include "ostype.h"

#define MAX_KEEPALIVE_COUNT 	99999
#define MAX_SERVER_STATE_INIT_SECOND	60
#define MAX_SERVER_STATE_DISCONNECTED_SECOND  30
#define MAX_SERVER_DOWN_COUNT	10


using namespace std;

enum ServerRole
{
	SERVER_ROLE_WORK = 0,
	SERVER_ROLE_STANDBY,
};

enum ServerState
{
	SERVER_STATE_INIT = -1,
	SERVER_STATE_DOWN = 0,
	SERVER_STATE_DISCONNECTING,
	SERVER_STATE_DISCONNECTED,
	SERVER_STATE_DOWNING,
	SERVER_STATE_RUNNING,
};

class CServerEndpoint
{
public:
	CServerEndpoint();
	~CServerEndpoint();

	CServerEndpoint& operator=(const CServerEndpoint& endpoint);
	char* GetServerEndpointState(char* buffer);
	char* GetServerEndpointRole(char* buffer);
	
//private:
	bool		m_usedFlag;	//TODO:need to delete it
	int 		m_sockFd;
	string		m_strConnectIp;	//this as the endpoint index
	uint16_t	m_connectPort;
	
	string		m_strPublicIp;
	string 		m_strHostResourceId;	//Ucloud Host ResourceId
	string 		m_strHostRegion;		//Ucloud Host Region
	string 		m_strEIPResourceId;		//Ucloud EIP ResourceId
	string		m_strEIPRegion;			//Ucloud EIP Region
	string		m_strDBIp;
	string 		m_strDBName;

	ServerRole  m_serverRole;
	ServerState m_serverState;

	uint32_t  	m_keepAliveCount;
	uint32_t	m_updateTimer;
	int 		m_downCount;
};

#endif


