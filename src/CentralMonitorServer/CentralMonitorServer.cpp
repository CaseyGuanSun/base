/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 11:25
** Email        : caseyguan@posteritytech.com
** Filename     : CentralMonitorServer.cpp
** Description  : 
** ******************************************************/
#include<sys/types.h>
#include<sys/stat.h>
#include "CentralMonitorServer.h"
extern "C"
{
#include "ipcsock.h"
}
#include "sha1.h"

#define TAG "CentralMonitorServer"

CDebug* g_debug = NULL;
#define	MONITOR_CONFIG_FILE		"CentralMonitorServer.cfg"
static CCentralMonitorServer* CentralMonitorServer = NULL;

CCentralMonitorServer::CCentralMonitorServer(string programName, string cfgName):
	m_programName(programName)
   ,m_configName(cfgName)
   ,pProcessThread(NULL)
   ,pReceiveThread(NULL)
   ,pKeepaliveThread(NULL)
{
	m_logName = m_programName+".log";
}

CCentralMonitorServer::~CCentralMonitorServer()
{
	if(pKeepaliveThread)
	{
		delete pKeepaliveThread;
		pKeepaliveThread = NULL;
	}
	if(pReceiveThread)
	{
		delete pReceiveThread;
		pReceiveThread = NULL;
	}
	if(pProcessThread)
	{
		delete pProcessThread;
		pProcessThread = NULL;
	}
	if(g_debug)
	{
		delete g_debug;
		g_debug = NULL;
	}
}

bool CCentralMonitorServer::ReadConfig(const char* configFileName)
{
	char		tmpString[1024];
	int 		tmp;
	FILE		*fp;

	if(NULL == configFileName)
	{
		return false;
	}
	
	/* set the default values */
	m_logFlag = true;
	m_logLevel = 0;
	m_logDir = "/home/CentralMonitorServer/CentralMonitorServer/log";
	m_logDeleteDir = "/home/CentralMonitorServer/CentralMonitorServer/delete";
	m_keepAliveInterval = 3;
	m_keepAliveTimeout = 60;

	if((fp=fopen(configFileName,"r"))==NULL)
	{	
		fprintf(stderr,"Can't open config file:%s, use default data\n",configFileName);
		return false;
	} 
	
	if(getConfigFromFile(fp, "LOGDEBUG", tmpString))
	{
		tmp=atoi(tmpString);
		if(tmp)
		{
			m_logFlag= true;
		}else
		{
			m_logFlag= false;
		}
	}

	if(getConfigFromFile(fp, "DEBUGLEVEL", tmpString))
	{
		m_logLevel = atoi(tmpString);
	}

	if(getConfigFromFile(fp, "LOGDIR", tmpString))
	{
		m_logDir = tmpString;
	}

	if(getConfigFromFile(fp, "DELETEDIR", tmpString))
	{
		m_logDeleteDir = tmpString;
	}

	if(getConfigFromFile(fp, "LISTENIP", tmpString))
	{
		m_listenTcpIp = tmpString;
	}

	if(getConfigFromFile(fp, "LISTENPORT", tmpString))
	{
		m_listenTcpPort = atoi(tmpString);
	}

	if(getConfigFromFile(fp, "KEEPALIVE_INTERVAL", tmpString))
	{
		m_keepAliveInterval = atoi(tmpString);
	}

	if(getConfigFromFile(fp, "KEEPALIVE_TIMEOUT", tmpString))
	{
		m_keepAliveTimeout = atoi(tmpString);
	}

	//Ucoud Info
	if(getConfigFromFile(fp, "APIINTERFACE", tmpString))
	{
		m_apiInfo.apiInterface = tmpString;
	}
	if(getConfigFromFile(fp, "APIPUBLICKEY", tmpString))
	{
		m_apiInfo.publicKey = tmpString;
	}
	if(getConfigFromFile(fp, "APIRIVATEKEY", tmpString))
	{
		m_apiInfo.privateKey = tmpString;
	}
	
	return true;
}
void CCentralMonitorServer::PrintConfg()
{
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "LOGDEBUG:%s", m_logFlag?"YES":"NO");
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "DEBUGLEVEL:%d", m_logLevel);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "LOGDIR:%s", m_logDir.c_str());
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "DELETEDIR:%s", m_logDeleteDir.c_str());
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "LISTENIP:%s", m_listenTcpIp.c_str());
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "LISTENPORT:%d", m_listenTcpPort);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "KEEPALIVE_INTERVAL:%d", m_keepAliveInterval);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "KEEPALIVE_TIMEOUT:%d", m_keepAliveTimeout);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "APIINTERFACE:%s", m_apiInfo.apiInterface.c_str());
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "APIPUBLICEKEY:%s", m_apiInfo.publicKey.c_str());
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "APIPRIVATEKEY:%s", m_apiInfo.privateKey.c_str());
}

bool CCentralMonitorServer::Initialize()
{
	bool res = false;

	res = ReadConfig(m_configName.c_str());
	if(!res)
	{
		printf("Read Config file failed\n");
		return res;
	}

	g_debug = CDebug::Create(LOG_LEVEL_VERBOSE, m_logName.c_str(), m_logDir.c_str(), m_logDeleteDir.c_str());
	if(g_debug == NULL)
	{
		printf("Debug Create failed, logName:%s, logDir:%s, logDeleteDir:%s\n", m_logName.c_str(), m_logDir.c_str(), m_logDeleteDir.c_str());
		return false;
	}
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Debug Create Success");
	PrintConfg();

	pProcessThread = CProcessThread::Create(&m_endpointManager, &m_apiInfo);
	if(NULL == pProcessThread)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Create Process Thread failed");
		return false;
	}
	
	pReceiveThread = CReceiveThread::Create(m_listenTcpIp, m_listenTcpPort, true, &m_endpointManager, pProcessThread);
	if(NULL == pReceiveThread)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Create Receive Thread failed, listen IP:%s, listen port:%d", m_listenTcpIp.c_str(), m_listenTcpPort);
		return false;
	}

	pKeepaliveThread = CKeepAliveThread::Create(m_keepAliveInterval, m_keepAliveTimeout, &m_endpointManager, pProcessThread);
	if(NULL == pKeepaliveThread)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Create keep alive thread failed");
		return false;
	}
	
	return true;
}
bool CCentralMonitorServer::Start()
{
	bool ret;

	ret = pProcessThread->Start();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, "TAG", "Process Thread start failed");
		return false;
	}

	ret = pKeepaliveThread->Start();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, "TAG", "Keep Alive Thread start failed");
		pProcessThread->Stop();
		return false;
	}

	ret = pReceiveThread->Start();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, "TAG", "Receive Thread start failed");
		pKeepaliveThread->Stop();
		pReceiveThread->Stop();
		return false;
	}

	return true;
}
bool CCentralMonitorServer::Stop()
{
	bool ret;

	ret = pProcessThread->Stop();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Thread stop failed");
		return false;
	}

	ret = pKeepaliveThread->Stop();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Keep Alive Thread stop failed");
		return false;
	}

	ret = pReceiveThread->Stop();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Receive Thread stop failed");
		return false;
	}

	return true;
}

void CCentralMonitorServer::TestSHA1(char* str, int len)
{
	unsigned char buffer[128];
	int i=0;
	char result[128];
		
	g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Test SHA1 str:%s, len:%d", str, len);
	memset(buffer, 0x00, sizeof(buffer));
	mbedtls_sha1((const unsigned char*)str, len, buffer);
	
	memset(result, 0x00, sizeof(result));
	for(i=0; i<20; i++)
	{
		sprintf(result, "%s%02x",result, buffer[i]);
	}
	if(!strcmp("64e0fe58642b75db052d50fd7380f79e6a0211bd", result))
	{
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "the SHA1 code is right");
	}else
	{
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "The SHA1 code is not right");
	}
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Test SHA1 result:%s,buffer:%s",  result, buffer);
}

void CCentralMonitorServer::TestGetHostInfo(string region, string hostId)
{
	pProcessThread->GetUHostInfo(region,hostId);
}


void gracefulTerminate(int sig)
{
	if(CentralMonitorServer)
	{
		CentralMonitorServer->Stop();
		delete CentralMonitorServer;
	}

	printf("CentralMonitorServer Graceful Terminate, Signal:%d\n", sig);
	exit(0);
}


int main(int argc, char* argv[])
{
	bool 	bRet = false;
	
	setProcessSignal(0);
	signal(SIGSEGV,gracefulTerminate);/* generate a core */
	signal(SIGTERM,gracefulTerminate); /* Terminate gracefully */

	umask(000);

	setProcessUlimit(65535);
	setProcessPriority(-20);

	if(argc>1)
	{
		CentralMonitorServer = new CCentralMonitorServer(argv[0], argv[1]);
	}
	else
	{
		CentralMonitorServer = new CCentralMonitorServer("CentralMonitorServer", MONITOR_CONFIG_FILE);
	}
	assert(NULL != CentralMonitorServer);

	bRet = CentralMonitorServer->Initialize();
	if(!bRet)
	{
		printf("Central Monitor Server Initialize failed");
		exit(0);
	}
	//char buffer[512] = "ActionCreateUHostInstanceCPU2ChargeTypeMonthDiskSpace10ImageIdf43736e1-65a5-4bea-ad2e-8a46e18883c2LoginModePasswordMemory2048NameHost01PasswordVUNsb3VkLmNuPublicKeyucloudsomeone@example.com1296235120854146120Quantity1Regioncn-north-0146f09bb9fab4f12dfc160dae12273d5332b5debe";
	//CentralMonitorServer->TestSHA1(buffer, strlen(buffer));

//	CentralMonitorServer->TestGetHostInfo("cn-north-03", "uhost-3m2lji");
//	myUsleep(10,0);

	
	bRet = CentralMonitorServer->Start();
	if(!bRet)
	{
		printf("Central Monitor Server Start failed");
		exit(0);
	}
	myUsleep(2, 0);

	//printf config info
	CentralMonitorServer->PrintConfg();
	
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Central Monitor Server Start success\n");
	while(1)
	{
		myUsleep(5,0);
	}
	
	return 0;
}





