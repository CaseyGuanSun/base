/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 11:25
** Email        : caseyguan@posteritytech.com
** Filename     : CentralMonitorServer.h
** Description  : Define Central Monitor Server Class
** ******************************************************/
#ifndef _CENTRAL_MONITOR_SERVER_H_
#define _CENTRAL_MONITOR_SERVER_H_

#include <string>
#include "ostype.h"
#include "Debug.h"
#include "ProcessThread.h"
#include "ServerEndpointManager.h"
#include "KeepAliveThread.h"
#include "ReceiveThread.h"
#include "ApiInfo.h"

using namespace std;

extern CDebug* g_debug;

class CKeepAliveThread;
class CProcessThread;
class CServerEndpintManager;
class CReceiveThread;

class CCentralMonitorServer
{
public:
	CCentralMonitorServer(string programName, string cfgName);
	~CCentralMonitorServer();

	bool ReadConfig(const char* configFileName);
	void PrintConfg();

	bool Initialize();
	bool Start();
	bool Stop();

//test
	void TestSHA1(char* str, int len);
	void TestGetHostInfo(string region, string hostId);
//end

private:
	string 		m_programName;
	string 		m_configName;

	bool		m_logFlag;
	uint32_t 	m_logLevel;
	string 		m_logDir;
	string 		m_logDeleteDir;
	string 		m_logName;

	string 		m_listenTcpIp;
	uint16_t	m_listenTcpPort;
	uint32_t	m_keepAliveInterval;
	uint32_t 	m_keepAliveTimeout;

	CServerEndpintManager m_endpointManager;
	CProcessThread* pProcessThread;
	CReceiveThread* pReceiveThread;
	CKeepAliveThread* pKeepaliveThread;

	ApiInfo			m_apiInfo;
};

#endif

