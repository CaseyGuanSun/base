/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-16 12:48
** Email        : caseyguan@posteritytech.com
** Filename     : ReceiveThread.cpp
** Description  : 
** ******************************************************/
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ReceiveThread.h"
#include "BaseThread.h"

#define TAG "ReceiveThread"


CReceiveThread* CReceiveThread::Create(string listenIp, uint16_t listenPort, bool bindFlag, CServerEndpintManager* pManager, CProcessThread* pThread)
{
	bool ret;
	CReceiveThread* ptr = new CReceiveThread(listenIp, listenPort, bindFlag, pManager, pThread);
	if(ptr == NULL)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Create Receive Thread failed");
		return NULL;
	}
	ret = ptr->Construct();
	if(!ret)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Init Receive Thread failed");
		delete ptr;
		return NULL;
	}
	return ptr;
}

bool CReceiveThread::ReceiveCbFunction(ThreadObj param)
{
	CReceiveThread* ptr = (CReceiveThread*)param;
	return ptr->TcpReceive();
}


CReceiveThread::CReceiveThread(string &listenIp, uint16_t  listenPort, bool bindFlag, CServerEndpintManager* pManager, CProcessThread* pThread):
	m_quitFlag(false)
	,m_bindFlag(bindFlag)
   ,m_listenIp(listenIp)
   ,m_listenPort(listenPort)
   ,m_receiveSockFd(-1)
   ,m_pThread(NULL)
   ,m_pEndpointManager(pManager)
   ,m_processThread(pThread)
   
{
}
	
CReceiveThread::~CReceiveThread()
{
	if(m_receiveSockFd> 0)
	{
		close(m_receiveSockFd);
	}
}

bool CReceiveThread::Construct()
{
	if(!m_bindFlag)		
	{
		m_receiveSockFd = createTcpSocket(NULL, m_listenPort, SOCKET_NON_BLOCK, 2); 
	}else
	{
		char temp[128];
		memset(temp, 0x00, sizeof(temp));
		sprintf(temp, "%s", m_listenIp.c_str());
		m_receiveSockFd = createTcpSocket(temp, m_listenPort, SOCKET_NON_BLOCK, 2);
	}

	if(m_receiveSockFd < 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Create TCP socket failed, listenip:%s, port:%d", m_listenIp.c_str(), m_listenPort);
		m_receiveSockFd = -1;
		return false;
	}
	else
	{
		if(-1 == listen(m_receiveSockFd, MAX_CONNECTED_SOCKETS))
		{
			g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Listen TCP socket failed, listenIp:%s, port:%d", m_listenIp.c_str(), m_listenPort);
			close(m_receiveSockFd);
			m_receiveSockFd = -1;
			return false;
		}
		else
		{
			g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Listen TCP socket success, listenIp:%s, port:%d", m_listenIp.c_str(), m_listenPort);		
		}
	}
	m_pThread = CBaseThread::Create(ReceiveCbFunction,this, PRIORITY_NORMAL, "ReceiveThread");
	return true;
}

bool CReceiveThread::Start()
{
	if(m_pThread)
	{
		return m_pThread->Start(m_threadId);
	}
	return false;
}

bool CReceiveThread::Stop()
{
	if(m_quitFlag)
	{
		return true;
	}
	m_quitFlag = true;
	if(m_pThread)
	{
		return m_pThread->Stop();
	}
	return false;
}


int CReceiveThread::ReceiveTcpMessage(int sockFd, char* msgBuffer, int bufLen)
{
	unsigned char	tcp_header[12];
	int 			ret;
	int 			msgBodyLen;
	int 			tmpLen;
	
	ret=recv(sockFd,tcp_header,12, 0);
	if(ret!=12)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR: recv the length:%d != 12,err=%d, error:%s\n", ret, errno, strerror(errno));
		return(-1);
	}
	if( tcp_header[0]!=0xFF ||
		tcp_header[1]!=0xFE ||
		tcp_header[2]!=0xFF ||
		tcp_header[3]!=0xFE ||
		tcp_header[4]!=0xFF ||
		tcp_header[5]!=0xFE ||
		tcp_header[6]!=0xFF ||
		tcp_header[7]!=0xFE
		)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR: Sync not right\n");
		return(-1);
	}
	memcpy(&tmpLen,&tcp_header[8],sizeof(int));
	msgBodyLen=ntohl(tmpLen);
	if(msgBodyLen>=MAXTCPPACKETLEN ||
		msgBodyLen<0 ||
		msgBodyLen >= bufLen)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR: msgBodyLen:%d\n", msgBodyLen);
		return(-1);
	}
	//	ret=recv(sockFd,msgBuffer,msgBodyLen,MSG_WAITALL);
	ret=recv(sockFd,msgBuffer,msgBodyLen, 0);
	if(ret!=msgBodyLen)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "ERROR: ret:%d != msgBodyLen:%d, err:%d, error:%s\n", ret, msgBodyLen, errno, strerror(errno));
		return(-1);
	}
	msgBuffer[msgBodyLen]=0;
	return(msgBodyLen);
}


bool CReceiveThread::TcpReceive()
{
	int epoll_fd;
	int efds;
	int i=0;
	int len;
	bool retFlag = false;
	struct epoll_event ev;
	struct epoll_event epoll_events[MAX_RECEIVE_EPOLL_EVENTS];
	struct sockaddr_in	remote_addr;
	socklen_t	addr_len ;
	int client_sockfd;
	PROCESS_MSG msg;
	char	buffer[MAX_TCP_MESSAGE_BUFFER];
	string  connectIp;
	uint16_t	connectPort;

	epoll_fd = epoll_create(MAX_RECEIVE_EPOLL_EVENTS);
	if(epoll_fd  < 0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "create tcp epoll failed\n");	
		return false;
	}

	ev.events = EPOLLIN|EPOLLET;
	ev.data.fd =  m_receiveSockFd;	
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, m_receiveSockFd, &ev)<0)
	{
		g_debug->DebugMsg(LOG_LEVEL_ERROR,TAG, "tcp epoll ctl error, index:%d\n", i);
		return false;
	}
	else
	{
		g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "add tcp socketFd to epoll success,sockFd:%d\n",m_receiveSockFd);
	}

	while(!m_quitFlag)
	{
		efds = epoll_wait(epoll_fd, epoll_events, MAX_EPOLL_EVENTS, -1 );
		if(efds < 0)
		{
			g_debug->DebugMsg(LOG_LEVEL_WARNING, TAG, "tcp epoll wait failed, errno:%d, error:%s\n", errno,strerror(errno));
			myUsleep(0, 200);
			continue;	
		}
		for(i=0; i<efds; i++)
		{
			if(epoll_events[i].data.fd == m_receiveSockFd)
			{
				//accept the connect sockFd
				addr_len = sizeof(remote_addr);
				memset(&remote_addr,0, sizeof(remote_addr));
				if((client_sockfd = accept(m_receiveSockFd, (struct sockaddr*)&remote_addr, &addr_len))<0)
				{
					g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "tcp addr:%s port:%d accept error,sockFd:%d, errno:%d, error:%s\n", client_sockfd, errno, strerror(errno));
					continue;	
				}
				
				//need to add this socket to connected table
				connectIp = inet_ntoa(remote_addr.sin_addr);
				connectPort = ntohs(remote_addr.sin_port);
				g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "accep new tcp socket, sockFd:%d, remoteIp:%s, remotePort:%d", client_sockfd, connectIp.c_str(), connectPort);
				m_pEndpointManager->AddServerEndpoint(client_sockfd, connectIp, connectPort);
			
				const int keepAlive = 1; //open keep alive feature
				const int keepIdle = 20; //
				const int keepInterval = 5;
				const int keepCount = 15;
	
				setsockopt(client_sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepAlive, sizeof(keepAlive));
				setsockopt(client_sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
				setsockopt(client_sockfd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepInterval, sizeof(keepInterval));
				setsockopt(client_sockfd, SOL_TCP, TCP_KEEPCNT, (void*)&keepCount, sizeof(keepCount));
	
				//ev.events = EPOLLIN|EPOLLET;
				ev.events = EPOLLIN|EPOLLET;
				ev.data.fd = client_sockfd;
				if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sockfd, &ev) == -1)
				{
					g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "tcp epoll add sockfd error, addr:%s, port:%d", connectIp.c_str(), connectPort);
					continue;	
				}
				g_debug->DebugMsg(LOG_LEVEL_INFO, TAG,"tcp epoll add new sockfd success, addr:%s, port:%d", connectIp.c_str(), connectPort);
							
			}
			else
			{
				retFlag = m_pEndpointManager->CheckServerEndpoint(epoll_events[i].data.fd);
				if(!retFlag)
				{
					g_debug->DebugMsg(LOG_LEVEL_WARNING, TAG, "can't find tcp sockfd from  connected talbe, i:%d, sockFd:%d", i, epoll_events[i].data.fd);
					close(epoll_events[i].data.fd);
					continue;	
				}
				else
				{
					memset(buffer, 0x00, sizeof(buffer));
					len = ReceiveTcpMessage(epoll_events[i].data.fd, buffer, MAX_TCP_MESSAGE_BUFFER);			
					if(len < 0)
					{
						if(errno == ECONNRESET ||
						   errno == ECONNABORTED ||
						   errno == ENETRESET ||
						   errno == ENETUNREACH ||
						   errno == ENETDOWN ||
						   errno == ETIMEDOUT)
						   /*
						if(errno == EAGAIN ||
						   errno == EWOULDBLOCK)
						{
							continue;
						}else*/
						{
							ev.events = EPOLLIN ;
							ev.data.fd = epoll_events[i].data.fd;
							if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd, &ev) == -1)
							{
								g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Error:tcp epoll delete sockfd error, index:%d, errno:%d error:%s\n",i, errno, strerror(errno) );
							}
							close(epoll_events[i].data.fd);//Added by casey for delete close_wait bug
					
							memset(&msg, 0x00, sizeof(PROCESS_MSG));
							msg.sockFd = epoll_events[i].data.fd;
							msg.msgType = MSG_TYPE_HOST;
							len = CreateAgentDownMsg(msg.msgBuf, sizeof(msg.msgBuf)-1);
							msg.msgLen = len;
							m_processThread->putProcessMessage(&msg);

						}
						continue;
					}
					else if(len == 0)
					{
						//the remote socket has closed so delete it from connected table
						ev.events = EPOLLIN ;
						ev.data.fd = epoll_events[i].data.fd;
						if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epoll_events[i].data.fd, &ev) == -1)
						{
							g_debug->DebugMsg(LOG_LEVEL_ERROR, TAG, "Error:tcp epoll delete sockfd error, index:%d, errno:%d error:%s\n",i, errno, strerror(errno) );
						}
						close(epoll_events[i].data.fd);//Added by casey for delete close_wait bug

						memset(&msg, 0x00, sizeof(PROCESS_MSG));
						msg.sockFd = epoll_events[i].data.fd;
						msg.msgType = MSG_TYPE_HOST;
						len = CreateAgentDownMsg(msg.msgBuf, sizeof(msg.msgBuf)-1);
						msg.msgLen = len;
						m_processThread->putProcessMessage(&msg);
						
						continue;
					}
					else
					{
						g_debug->DebugMsg(LOG_LEVEL_INFO, TAG, "Get Message:\n%s", buffer);
						//put message to process thread queue
						memset(&msg, 0x00, sizeof(PROCESS_MSG));
						msg.sockFd = epoll_events[i].data.fd;
						msg.msgType = MSG_TYPE_AGENT;
						strncpy(msg.msgBuf, buffer, sizeof(msg.msgBuf)-1);
						msg.msgLen = len;
						m_processThread->putProcessMessage(&msg);
					}
				}
			}
		}
	}
	return false;
}

int CReceiveThread::CreateAgentDownMsg(char* buffer, int bufLen)
{
	assert(buffer);
	int len = 0;
	len = sprintf(buffer, "SOCKETDOWN\r\n");
	return len;
}



