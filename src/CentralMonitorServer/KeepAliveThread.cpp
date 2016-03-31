/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-17 18:15
** Email        : caseyguan@posteritytech.com
** Filename     : KeepAliveThread.cpp
** Description  : Implement Keep Alive Thread Class
** ******************************************************/
#include "KeepAliveThread.h"
extern "C"
{
#include "ipcsock.h"
}

#define TAG "KeepAliveThread"

CKeepAliveThread* CKeepAliveThread::Create(unsigned int intervalSec, unsigned int timeoutSec, CServerEndpintManager* pManager, CProcessThread* pProcessThread)
{
	bool ret;
	CKeepAliveThread* ptr = new CKeepAliveThread(intervalSec, timeoutSec, pManager, pProcessThread);
	if(NULL == ptr)
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


CKeepAliveThread::CKeepAliveThread(unsigned int intervalSec, unsigned int timeoutSec, CServerEndpintManager* pManager, CProcessThread* pProcessThread):
    m_quitFlag(false)
   ,pBaseThread(NULL)
   ,m_intervalSecond(intervalSec)
   ,m_timeoutSecond(timeoutSec)
   ,m_endpointManager(pManager)
   ,m_processThread(pProcessThread)
{
}

CKeepAliveThread::~CKeepAliveThread()
{

}

bool CKeepAliveThread::KeepAliveCb(void* param)
{
	assert(param);
	CKeepAliveThread* ptr = (CKeepAliveThread*)param;
	return ptr->RunKeepAlive();
}


bool CKeepAliveThread::Init()
{
	pBaseThread = CBaseThread::Create(KeepAliveCb, this, PRIORITY_NORMAL, "KeepAliveThread");
	if(pBaseThread == NULL)
	{
		return false;
	}
	return true;
}
bool CKeepAliveThread::Start()
{
	unsigned int threadId = 0;
	bool ret = false;
	if(pBaseThread)
	{
		ret =pBaseThread->Start(threadId);
	}
	return ret;
}

bool CKeepAliveThread::Stop()
{
	bool ret = false;

	if(m_quitFlag)
	{
		return true;
	}
	
	if(pBaseThread)
	{
		ret = pBaseThread->Stop();
	}

	return ret;
}

bool CKeepAliveThread::RunKeepAlive()
{
	int i=0;
	int count = 0;
	CServerEndpoint* endpoint;
	
	while(!m_quitFlag)
	{
		count = m_endpointManager->GetServerEndpointCount();
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Now Server Endpoint count:%d", count);
		m_endpointManager->PrintAllServerEndpoint();
		for(i=0; i<count; i++)
		{
			endpoint= m_endpointManager->GetServerEndpoint(i);
			if(endpoint != NULL)
			{
				ProcessServerEndpoint(endpoint);
			}else
			{
				g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "GetServerEndpoint failed, index:%d", i);
			}
		}

		//sleep 3 second
		myUsleep(m_intervalSecond, 0);
	}
	return false;
}

int CKeepAliveThread::SendKeepAliveMessage(CServerEndpoint* endpoint)
{
	char buffer[256];
	int msgLen;
	int ret;

	assert(endpoint);
	
	memset(buffer, 0x00, sizeof(buffer));
	msgLen = sprintf(buffer, "KEEPALIVE\r\nSYNC:%d\r\n", endpoint->m_keepAliveCount);
	endpoint->m_keepAliveCount++;
	if(endpoint->m_keepAliveCount >= MAX_KEEPALIVE_COUNT)
	{
		endpoint->m_keepAliveCount = 0;
	}
	ret = sendTcpMessage(endpoint->m_sockFd, buffer, msgLen);
	if(ret < 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "SendKeepAliveMessage failed, Message:\n%s", buffer);
		return ret;
	}
	g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Send Message:\n%s", buffer);
	return 0;
}

int CKeepAliveThread::ProcessServerEndpoint(CServerEndpoint* endpoint)
{
	time_t currentTime;
	PROCESS_MSG msg;
	
	assert(endpoint);

	currentTime = getCurrentTime_t();
	if(SERVER_STATE_INIT == endpoint->m_serverState)
	{
		SendKeepAliveMessage(endpoint);
		if(currentTime - endpoint->m_updateTimer > MAX_SERVER_STATE_INIT_SECOND)
		{
			//maybe this connection is an attack, now delete it
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "The server endpoint don't register in time, now delete it, currentTime:%d, endpoint updateTimer:%d", currentTime, endpoint->m_updateTimer);
			m_endpointManager->DeleteServerEndpoint(endpoint->m_sockFd);
		}
	}else if(SERVER_STATE_RUNNING == endpoint->m_serverState)
	{
		SendKeepAliveMessage(endpoint);
		if(currentTime - endpoint->m_updateTimer > m_timeoutSecond)
		{
			//keepalive time out 
			//so maybe this agent is down
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "The server didn't receive Keep alive ack more than %d second", m_timeoutSecond);
			endpoint->m_serverState = SERVER_STATE_DISCONNECTED;
			if(endpoint->m_serverRole == SERVER_ROLE_WORK)
			{
				memset(&msg, 0x00, sizeof(PROCESS_MSG));
				CreateServerDownMsg(&msg, endpoint);
				m_processThread->putProcessMessage(&msg);
			}
		}
	}else if(SERVER_STATE_DISCONNECTING == endpoint->m_serverState)
	{
		if(currentTime - endpoint->m_updateTimer > MAX_SERVER_STATE_DISCONNECTED_SECOND) 
		{
			//The server is disconnected for long time
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "The server endpoint is disconnected more than %d second", MAX_SERVER_STATE_DISCONNECTED_SECOND);
			endpoint->m_serverState = SERVER_STATE_DISCONNECTED;	
			if(endpoint->m_serverRole == SERVER_ROLE_WORK)
			{
				memset(&msg, 0x00, sizeof(PROCESS_MSG));
				CreateServerDownMsg(&msg, endpoint);
				m_processThread->putProcessMessage(&msg);
			}
		}
	}else if(SERVER_STATE_DOWNING == endpoint->m_serverState)
	{
		SendKeepAliveMessage(endpoint);
	
		if(endpoint->m_downCount >= MAX_SERVER_DOWN_COUNT)
		{
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "The server endpoint is downing, count:%d", endpoint->m_downCount);
			if(endpoint->m_serverRole == SERVER_ROLE_STANDBY)
			{
				g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "This server is standby, so do nothint, set the state Down");
				endpoint->m_serverState = SERVER_STATE_DOWN;
				return 0;
			}else
			{
				endpoint->m_serverState = SERVER_STATE_DOWN;
				memset(&msg, 0x00, sizeof(PROCESS_MSG));
				CreateServerDownMsg(&msg, endpoint);
				m_processThread->putProcessMessage(&msg);
			}
		}
	}
	return 0;
}

int CKeepAliveThread::CreateServerDownMsg(PROCESS_MSG* msg, CServerEndpoint* endpoint)
{
	assert(msg);
	
	msg->sockFd = endpoint->m_sockFd;
	msg->msgType = MSG_TYPE_HOST;
	msg->msgLen = sprintf(msg->msgBuf, "SERVERDOWN\r\nCONNECTIP:%s\r\n", endpoint->m_strConnectIp.c_str());
	
	return msg->msgLen;
}

