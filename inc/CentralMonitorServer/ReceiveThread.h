/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 12:48
** Email        : caseyguan@posteritytech.com
** Filename     : ReceiveThread.h
** Description  : Define Receive Thread Class
** ******************************************************/
#ifndef _RECEIVE_THREAD_H_
#define _RECEIVE_THREAD_H_
#include "ostype.h"
#include "ProcessThread.h"
#include "ServerEndpointManager.h"

using namespace std;

#define MAX_CONNECTED_SOCKETS		60
#define MAX_RECEIVE_EPOLL_EVENTS	1024
#define MAX_EPOLL_EVENTS 			1024
#define MAX_TCP_MESSAGE_BUFFER		6000

class CProcessThread;

class CReceiveThread
{
public:
	static CReceiveThread* Create(string listenIp, uint16_t listenPort, bool bindFlag, CServerEndpintManager* pManager, CProcessThread* pThread);
	static bool ReceiveCbFunction(ThreadObj param);
	CReceiveThread(string& listenIp, uint16_t  listenPort, bool bindFlag, CServerEndpintManager* pManager, CProcessThread* pThread);
	~CReceiveThread();
	
	bool Start();
	bool Stop();
private:
	bool Construct();
	bool TcpReceive();
	//return msg length
	int ReceiveTcpMessage(int sockFd, char* msgBuffer, int bufLen);
	int CreateAgentDownMsg(char* buffer, int bufLen);
private:
	bool			m_quitFlag;
	bool 			m_bindFlag;
	string 			m_listenIp;
	uint16_t		m_listenPort;
	int 			m_receiveSockFd;
	CBaseThread		*m_pThread;
	unsigned int 	m_threadId;
	CServerEndpintManager* m_pEndpointManager;
	CProcessThread* m_processThread;
};

#endif

