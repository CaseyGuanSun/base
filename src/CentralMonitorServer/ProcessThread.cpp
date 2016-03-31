/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 16:50
** Email        : caseyguan@posteritytech.com
** Filename     : ProcessThread.cpp
** Description  : Implement the Process Thread Class
** ******************************************************/
#include "ProcessThread.h"
#include "RequestParam.h"
#include "sha1.h"
#include "JsonApi.h"

#define TAG "ProcessThread"

CProcessThread* CProcessThread::Create(CServerEndpintManager* pManager, ApiInfo* apiInfo)
{
	bool ret;
	CProcessThread* ptr = new CProcessThread(pManager, apiInfo);
	if(ptr == NULL)
	{
		return NULL;
	}
	ret = ptr->Init();
	if(!ret)
	{
		delete ptr;
		return NULL;
	}
	return ptr;
}


bool CProcessThread::ProcessCbFunction(ThreadObj param)
{
	CProcessThread* ptr = (CProcessThread*)param;
	return ptr->ProcessFunction();
}

CProcessThread::CProcessThread(CServerEndpintManager* pManager, ApiInfo* apiInfo):
	m_bQuitFlag(false)
   ,m_pServerEndpointManager(pManager)
   ,m_pThread(NULL)
   ,m_apiInfo(apiInfo)
   ,m_pCondition(NULL)
   ,m_start(0)
   ,m_end(0)
   ,m_count(0)
{
}

CProcessThread::~CProcessThread()
{
	m_msgQueue.clear();
}

bool CProcessThread::Start()
{
	if(m_pThread)
	{
		return m_pThread->Start(m_threadId);
	}
	return false;
}

bool CProcessThread::Stop()
{
	if(m_pThread)
	{
		return m_pThread->Stop();
	}
	return false;
}


bool CProcessThread::Init()
{	
	PROCESS_MSG msg;
	int i=0;
	
	for(i=0; i< MAX_PROCESS_MSG; i++)
	{
		m_msgQueue.push_back(msg);
	}
	m_pCondition = CCondition::Create(&m_lock);
	if(NULL == m_pCondition)
	{
		return false;
	}

	m_pThread = CBaseThread::Create(ProcessCbFunction,this, PRIORITY_NORMAL,"ProcessThread");
	return true;
}


int CProcessThread::putProcessMessage(PROCESS_MSG *msg)
{
	PROCESS_MSG* pMsg = NULL;
	
	CriticalSectionScoped cs(&m_lock);
	
	if(m_end >= m_start)
	{
		if((m_end-m_start+1)>=MAX_PROCESS_MSG)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR:Thre process thread message queue is full!!!");
			return -1;
		}
		pMsg = &m_msgQueue[m_end];
		memcpy(pMsg, msg, sizeof(PROCESS_MSG));
		m_end++;
		m_count++;
		if(m_end >= MAX_PROCESS_MSG)
		{
			m_end = 0;
		}
	}else
	{
		if(m_start- m_end == 1)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR:Thre process thread message queue is over full!!!");
			return -1;
		}
		pMsg = &m_msgQueue[m_end];
		memcpy(pMsg, msg, sizeof(PROCESS_MSG));
		m_end++;
		m_count++;
		if(m_end >= MAX_PROCESS_MSG)
		{
			m_end = 0;
		}
	}
	m_pCondition->Notify();
	return 0;
}

int CProcessThread::getProcessMessage(PROCESS_MSG *msg)
{
	PROCESS_MSG* pMsg = NULL;

	m_lock.Lock();
	while(m_count<= 0)
	{
		m_pCondition->Wait();
	}
	pMsg = &m_msgQueue[m_start];
	memcpy(msg, pMsg, sizeof(PROCESS_MSG));
	m_start++;
	if(m_start>= MAX_PROCESS_MSG)
	{
		m_start = 0;
	}
	m_count--;
	m_lock.Unlock();
	return 0;
}

bool CProcessThread::ProcessFunction()
{
	PROCESS_MSG msg;

	memset(&msg, 0x00, sizeof(PROCESS_MSG));

	while(!m_bQuitFlag)
	{
		getProcessMessage(&msg);
		if(m_bQuitFlag)
		{
			break;
		}

		ProcessMsg(&msg);
	}
	return false;
}

bool CProcessThread::ProcessMsg(PROCESS_MSG* msg)
{
	char tmpBuf[128];
	bool res = false;

	assert(msg);

	getMessageCommand(msg->msgBuf, tmpBuf);

	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG,"Process Message:\n%s", msg->msgBuf);

	if(!strncmp(tmpBuf, "REGISTER", strlen("REGISTER")))
	{
		res = ProcessRegisterMsg(msg);
		if(res)
		{
			//register success
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Process Register Message success");
			SendRegisterAckMsg(msg->sockFd, 1);
		}else
		{
			//register failed
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Register Message failed");
			SendRegisterAckMsg(msg->sockFd, 0);
		}
	}else if(!strncmp(tmpBuf, "KEEPALIVEACK", strlen("KEEPALIVEACK")))
	{
		res = ProcessKeepAliveAckMsg(msg);
		if(!res)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Keep Alive  Ack Message failed");
		}
	}else if(!strncmp(tmpBuf, "STARTWORKACK", strlen("STARTWORKACK")))
	{
		res = ProcessStartWorkAckMsg(msg);
		if(!res)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Start Work Ack message failed");
		}
	}else if(!strncmp(tmpBuf, "SOCKETDOWN", strlen("SOCKETDOWN")))
	{
		res = ProcessSocketDownMsg(msg);
		if(!res)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Socket Down Message failed");
		}
	}else if(!strncmp(tmpBuf, "SERVERDOWN", strlen("SERVERDOWN")))
	{
		res = ProcessServerDownMsg(msg);
		if(!res)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process Server Down Message failed");
		}
	}else
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "The Command is not right, command:%s", tmpBuf);
	}
	return res;
}

bool CProcessThread::ProcessRegisterMsg(PROCESS_MSG* msg)
{
	char tmpBuf[256];
	char  hostRegion[128];
	char publicIp[128];
	char hostResourceId[128];
	char eipResourceId[128];
	char eipRegion[128];
	char dbIp[128];
	char dbName[128];
	int serverRole;
	ServerRole role;
	int serverState;
	ServerState state;
	bool res;
	
	assert(msg);

	memset(tmpBuf, 0x00, sizeof(tmpBuf));
	if(-1 == getMessageValue("HOSTREGION", msg->msgBuf, hostRegion))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG,"Can't get HOSTREGION header, from msg:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("PUBLICIP", msg->msgBuf, publicIp))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get PUBLICIP header, from msg:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue( "HOSTRESOURCEID", msg->msgBuf, hostResourceId))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get HOSTRESOURCEID heaer, from msg:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("EIPRESOURCEID", msg->msgBuf, eipResourceId))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get EIPRESOURCEID header, from:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("EIPREGION", msg->msgBuf,eipRegion))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get EIPREGION header, from:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("DBIP",msg->msgBuf, dbIp))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get DBIP header, from:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("DBNAME", msg->msgBuf, dbName))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get DBNAME header, msg:\n%s", msg->msgBuf);
		return false;
	}

	if(-1 == getMessageValue("ROLE",msg->msgBuf, tmpBuf))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get ROLE header, from:\n%s", msg->msgBuf);
		return false;
	}
	serverRole = atoi(tmpBuf);
	if(serverRole == 0)
	{
		role = SERVER_ROLE_STANDBY;
	}else
	{
		role = SERVER_ROLE_WORK;
	}

	//state
	if(-1 == getMessageValue("STATE", msg->msgBuf, tmpBuf))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get STATE header, from:\n%s", msg->msgBuf);
		return false;
	}
	serverState = atoi(tmpBuf);
	if(serverState == 0)
	{
		state = SERVER_STATE_RUNNING; 
	}else
	{
		state = SERVER_STATE_DOWN;
	}
	res = m_pServerEndpointManager->UpdateServerEndpointRegister(msg->sockFd, hostRegion, publicIp, hostResourceId, eipResourceId, eipRegion, dbIp, dbName, role, state);
	if(!res)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Update Server Endpoint Register failed");
	}else
	{
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Update Server Endpont success,HostRegin:%s, publicIp:%s, hostResourceId:%s ,"
					" eipResourceId:%s, eipRegion:%s, dbIp:%s, dbName:%s, serverRole:%d",
			hostRegion, publicIp,hostResourceId, eipResourceId, eipRegion, dbIp, dbName, serverRole);
	}
	return res;
}

bool CProcessThread::ProcessSocketDownMsg(PROCESS_MSG* msg)
{
	CServerEndpoint *pEndpoint = NULL;
	bool ret;
	
	assert(msg);

	pEndpoint = m_pServerEndpointManager->GetServerEndpointBySockFd(msg->sockFd);

	if(NULL == pEndpoint)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process socket down, can't get Server Endpoint by socket:%d", msg->sockFd);
		return false;
	}else
	{
		//set the server state as SERVER_STATE_DISCONNECTED
		ret = m_pServerEndpointManager->UpdateServerEndpointState(msg->sockFd, SERVER_STATE_DISCONNECTED);
		if(!ret)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Process socket down, update endpoint failed, sockFd:%d", msg->sockFd);
		}
		return ret;
	}
}


bool CProcessThread::ProcessKeepAliveAckMsg(PROCESS_MSG* msg)
{
	char tmpBuf[256];
	int status = 0;
	bool ret;
	ServerState state;

	assert(msg);

	memset(tmpBuf, 0x00, sizeof(tmpBuf));
	if(-1 == getMessageValue("STATUS", msg->msgBuf, tmpBuf))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG,"Can't get STATUS header, from msg:\n%s", msg->msgBuf);
		return false;
	}

	status = atoi(tmpBuf);
	if(status == 0)
	{
		state = SERVER_STATE_RUNNING;
	}else
	{
		state = SERVER_STATE_DOWN;
	}
	
	ret = m_pServerEndpointManager->UpdateServerEndpointState(msg->sockFd, state);
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Update Server Endpoint state failed");
	}
	
	if(status != 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Warning: The Keep Alive state is down:%d", status);
	}
	return true;
}

bool CProcessThread::ProcessStartWorkAckMsg(PROCESS_MSG* msg)
{
	char tmpBuf[256];
	int status;

	assert(msg);

	memset(tmpBuf, 0x00, sizeof(tmpBuf));
	if(-1 == getMessageValue("STATUS", msg->msgBuf, tmpBuf))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Can't get STATUS header, from msg:\n%s", msg->msgBuf);
		return false;
	}

	status = atoi(tmpBuf);
	if(!status)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Warning: Start Work failed");
	}else
	{	
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Start Work success");
	}
	return true;
}

bool CProcessThread::ProcessServerDownMsg(PROCESS_MSG* msg)
{
	CServerEndpoint *pEndpoint = NULL;
	bool ret = false;
	char serverIp[128];
	string strIp;
	assert(msg);

	memset(serverIp, 0x00, sizeof(serverIp));
	if(-1 == getMessageValue("CONNECTIP", msg->msgBuf, serverIp))
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG,"Can't get SERVERIP header, from msg:\n%s", msg->msgBuf);
		return false;
	}
	strIp = serverIp;
	pEndpoint = m_pServerEndpointManager->GetServerEndpointByIp(strIp);
	
	if(NULL == pEndpoint)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ProcessServerDownMsg error, can't get Server Endpoint by connectIp:%s", strIp.c_str());
		return false;
	}else
	{
		ret = ChangeFloatIp(pEndpoint);
		if(!ret)
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ProcessServerDownMsg error, ChangeFloatIp failed, connectIp:%s", strIp.c_str());
		}
		return ret;
	}
		
}


bool CProcessThread::BindEIP(string region, string eipId, string resourceType, string resouceId)
{
	map<string,RequestParam> paramMap;
	RequestParam param;
	bool res;

	//Action
	param.RequestName = "Action";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "BindEIP");
	paramMap.insert(make_pair(param.RequestName, param));

	//PublicKey
	param.RequestName = "PublicKey";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, m_apiInfo->publicKey.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//Region
	param.RequestName = "Region";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, region.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//EIPId
	param.RequestName = "EIPId";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, eipId.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//ResourceType
	param.RequestName = "ResourceType";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, resourceType.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//ResourceId
	param.RequestName = "ResourceId";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, resouceId.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	res = SendPostRequest(paramMap);
	return res;
}

bool CProcessThread::UnbindEIP(string region, string eipId, string resourceType, string resouceId)
{
	map<string,RequestParam> paramMap;
	RequestParam param;
	bool res;

	//Action
	param.RequestName = "Action";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "UnBindEIP");
	paramMap.insert(make_pair(param.RequestName, param));

	//PublicKey
	param.RequestName = "PublicKey";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, m_apiInfo->publicKey.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//Region
	param.RequestName = "Region";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, region.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//EIPId
	param.RequestName = "EIPId";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, eipId.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//ResourceType
	param.RequestName = "ResourceType";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, resourceType.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//ResourceId
	param.RequestName = "ResourceId";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, resouceId.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	res = SendPostRequest(paramMap);

	return res;
}

bool CProcessThread::GetUHostInfo(string region, string hostId)
{
	map<string,RequestParam> paramMap;
	RequestParam param;
	bool res;
  //  char buffer[1024];
//	int len;
//	unsigned char shaResult[40];
//	char strShaResult[50];
//	string strResponse;
//	int ret;
//	char split[10];

	//Action
	param.RequestName = "Action";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "%s", "DescribeUHostInstance");
	paramMap.insert(make_pair(param.RequestName, param));

	//PublicKey
	param.RequestName = "PublicKey";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "%s", m_apiInfo->publicKey.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//Region
	param.RequestName = "Region";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "%s", region.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	//UHostIds.n
	param.RequestName = "UHostIds.0";
	param.dataType = DATA_TYPE_STRING;
	sprintf(param.RequestData.strData, "%s", hostId.c_str());
	paramMap.insert(make_pair(param.RequestName, param));

	res = SendPostRequest(paramMap);
/*
	memset(buffer, 0x00, sizeof(buffer));
	map<string, RequestParam>::iterator iter;
	for(iter = paramMap.begin(); iter != paramMap.end(); iter++)
	{
		sprintf(buffer, "%s%s%s", buffer, iter->second.RequestName.c_str(), iter->second.RequestData.strData);
	}
	sprintf(buffer, "%s%s", buffer, m_apiInfo->privateKey.c_str());
	len = strlen(buffer);
	memset(shaResult, 0x00, sizeof(shaResult));
	mbedtls_sha1((const unsigned char*)buffer, len, shaResult);
	memset(strShaResult, 0x00, sizeof(strShaResult));
	int i=0;
	for(i=0; i<20; i++)
	{
		sprintf(strShaResult, "%s%02x",strShaResult, shaResult[i]);
	}
	
	//Create Get/Post URL
	memset(buffer, 0x00, sizeof(buffer));

	//Url
	sprintf(buffer, "%s", m_apiInfo->apiInterface.c_str());
	for(iter = paramMap.begin(); iter != paramMap.end(); iter++)
	{
		if(iter == paramMap.begin())
		{
			sprintf(split, "?");
		}else
		{
			sprintf(split, "&");
		}
	
		if(iter->second.dataType == DATA_TYPE_STRING)
		{
			string temp = iter->second.RequestData.strData;
			sprintf(buffer, "%s%s%s=%s", buffer, split, iter->second.RequestName.c_str(), httpClient.UrlEncode(temp).c_str());
		}else if(iter->second.dataType == DATE_TYPE_INTEGER)
		{	
			sprintf(buffer, "%s%s%s=%d", buffer, split, iter->second.RequestName.c_str(), iter->second.RequestData.iData);
		}else if(iter->second.dataType == DATE_TYPE_FLOAT)
		{
			sprintf(buffer, "%s%s%s=%f", buffer, split, iter->second.RequestName.c_str(), iter->second.RequestData.fData);
		}
	}
	sprintf(buffer, "%s&Signature=%s", buffer, strShaResult);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Http Client url:%s", buffer);
	//httpClient.SetDebug(true);
	ret= httpClient.Get(buffer, strResponse);
//	ret= httpClient.Get("www.baidu.com", strResponse);
//	snprintf(buffer,1024-1, "%s", strResponse.c_str());
//	buffer[1000] = '\0';
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "http client get response, ret:%d:%s", ret, strResponse.c_str());
*/
	return res;
}

bool CProcessThread::SendPostRequest(map<string,RequestParam> &paramMap)
{
 	char buffer[1024];
	int len;
	unsigned char shaResult[40];
	char strShaResult[50];
	string strResponse;
	int ret;
	char split[10];
	bool res;

	memset(buffer, 0x00, sizeof(buffer));
	map<string, RequestParam>::iterator iter;
	for(iter = paramMap.begin(); iter != paramMap.end(); iter++)
	{
		sprintf(buffer, "%s%s%s", buffer, iter->second.RequestName.c_str(), iter->second.RequestData.strData);
	}
	sprintf(buffer, "%s%s", buffer, m_apiInfo->privateKey.c_str());
	len = strlen(buffer);
	memset(shaResult, 0x00, sizeof(shaResult));
	mbedtls_sha1((const unsigned char*)buffer, len, shaResult);
	memset(strShaResult, 0x00, sizeof(strShaResult));
	int i=0;
	for(i=0; i<20; i++)
	{
		sprintf(strShaResult, "%s%02x",strShaResult, shaResult[i]);
	}
	
	//Create Get/Post URL
	memset(buffer, 0x00, sizeof(buffer));

	//Url
	sprintf(buffer, "%s", m_apiInfo->apiInterface.c_str());
	for(iter = paramMap.begin(); iter != paramMap.end(); iter++)
	{
		if(iter == paramMap.begin())
		{
			sprintf(split, "?");
		}else
		{
			sprintf(split, "&");
		}
	
		if(iter->second.dataType == DATA_TYPE_STRING)
		{
			string temp = iter->second.RequestData.strData;
			sprintf(buffer, "%s%s%s=%s", buffer, split, iter->second.RequestName.c_str(), httpClient.UrlEncode(temp).c_str());
		}else if(iter->second.dataType == DATE_TYPE_INTEGER)
		{	
			sprintf(buffer, "%s%s%s=%d", buffer, split, iter->second.RequestName.c_str(), iter->second.RequestData.iData);
		}else if(iter->second.dataType == DATE_TYPE_FLOAT)
		{
			sprintf(buffer, "%s%s%s=%f", buffer, split, iter->second.RequestName.c_str(), iter->second.RequestData.fData);
		}
	}
	sprintf(buffer, "%s&Signature=%s", buffer, strShaResult);
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Http Client url:%s", buffer);
	//httpClient.SetDebug(true);
	ret= httpClient.Get(buffer, strResponse);
//	ret= httpClient.Get("www.baidu.com", strResponse);
//	snprintf(buffer,1024-1, "%s", strResponse.c_str());
//	buffer[1000] = '\0';
	if(ret != 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Http Client Get failed, ret:%d", ret);
		return false;
	}
	ret = 0;
	res = GetIntValue(strResponse.c_str(), "RetCode", &ret);
	if(!res)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Http Client get Response RetCode failed, respone:%s", strResponse.c_str());
		return false;
	}
	if(ret != 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Http Client get response success, but return failed, response:%s", strResponse.c_str());
		return false;
	}
	
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "http client get response, ret:%d:%s", ret, strResponse.c_str());
	return true;
}

bool CProcessThread::SendStartWorkMsg(int sockFd, string &publicIp, string &eipResourceId, string &eipRegion, string &dbIp, string &dbName)
{
	int ret;
	char buffer[1024];
	int msgLen = 0;

	memset(buffer, 0x00, sizeof(buffer));
	addSipMessageHeaders(buffer, "STARTWORK");
	addSipMessageHeaders(buffer, "PUBLICIP:%s", publicIp.c_str());
	addSipMessageHeaders(buffer, "EIPRESOURCEID:%s", eipResourceId.c_str());
	addSipMessageHeaders(buffer, "EIPREGION:%s", eipRegion.c_str());
	addSipMessageHeaders(buffer, "DBIP:%s", dbIp.c_str());
	addSipMessageHeaders(buffer, "DBNAME:%s", dbName.c_str());
	msgLen = strlen(buffer);

	ret = sendTcpMessage(sockFd, buffer, msgLen);
	if(ret < 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "send start work tcp failed\n%s", buffer);
		return false;
	}
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG,"Send Message:\n%s", buffer);
	return true;
}

bool CProcessThread::SendRegisterAckMsg(int sockFd, int status)
{
	int ret;
	char buffer[1024];
	int msgLen = 0;

	memset(buffer, 0x00, sizeof(buffer));
	addSipMessageHeaders(buffer, "REGISTERACK");
	addSipMessageHeaders(buffer, "STATUS:%d", status);
	msgLen = strlen(buffer);
	ret = sendTcpMessage(sockFd, buffer, msgLen);
	if(ret < 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "send register ack tcp failed, msg:\n%s", buffer);
		return false;
	}
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Send Register Message Ack:\n%s", buffer);
	return true;
}


bool CProcessThread::ChangeFloatIp(CServerEndpoint* oldEndpoint)
{
	CServerEndpoint* newEndpoint = NULL;
	bool ret;

	newEndpoint = m_pServerEndpointManager->GetNewStandbyServer();
	if(newEndpoint == NULL)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Change Float IP failed, can't get New Standby server");
		return false;		
	}

//	oldEndpoint->m_serverState = SERVER_STATE_DOWN;

	//1.Unbind EIP
	ret= UnbindEIP(oldEndpoint->m_strEIPRegion, oldEndpoint->m_strEIPResourceId, RESOURCE_TYPE, oldEndpoint->m_strHostResourceId);
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Unbind old IP failed");
		return ret;
	}
	//2.Bind EIP
	ret = BindEIP(oldEndpoint->m_strEIPRegion, oldEndpoint->m_strEIPResourceId, RESOURCE_TYPE, newEndpoint->m_strHostResourceId);
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Bind Old IP failed");
		return ret;
	}
	newEndpoint->m_strPublicIp = oldEndpoint->m_strPublicIp;

	//3.Unbind New EIP
	ret = UnbindEIP(newEndpoint->m_strEIPRegion, newEndpoint->m_strEIPResourceId, RESOURCE_TYPE, newEndpoint->m_strHostResourceId);
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "UnbindEIP New IP failed"); 
		return ret;
	}

	//4.Bind New IP
	ret = BindEIP(newEndpoint->m_strEIPRegion, newEndpoint->m_strEIPResourceId, RESOURCE_TYPE, oldEndpoint->m_strHostResourceId);
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Bind new IP failed");
		return ret;
	}
	oldEndpoint->m_strPublicIp = newEndpoint->m_strPublicIp;

	//5.Send start work message
	SendStartWorkMsg(newEndpoint->m_sockFd, oldEndpoint->m_strPublicIp, oldEndpoint->m_strEIPResourceId, oldEndpoint->m_strEIPRegion, oldEndpoint->m_strDBIp, oldEndpoint->m_strDBName);

	return true;
}



