#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdlib.h>
#include <stdio.h> 
#include "ostype.h"
#include "Lock.h"
#include "BaseThread.h"
#include "Event.h"
#include "Condition.h"

#define MAX_LOGNAME_LENGTH  	512
#define MAX_DIR_LENGTH			512
#define MAX_LOG_LINES 			20000
#define MAX_LOG_BUF_LENGTH      2048

#define LOG_QUEUE_LENGTH		1000
#define LOG_QUEUE_ARRAY			2


//log level enum
typedef enum
{		
	LOG_LEVEL_NON =0,		
	LOG_LEVEL_ERROR,		
	LOG_LEVEL_WARNING,		
	LOG_LEVEL_INFO,		
	LOG_LEVEL_DEBUG,		
	LOG_LEVEL_VERBOSE
}LOG_LEVEL;


typedef struct log_message
{	
	LOG_LEVEL log_level;	
	int message_len;	
	char message_buf[MAX_LOG_BUF_LENGTH+1];
}log_message;


class CDebug
{
public:
	static CDebug* Create(LOG_LEVEL level, const char* logName, const char* logDir, const char* deleteDir);
	CDebug();
	~CDebug();
	static bool CbThread(void* context);
	bool 	CbRunImpl();
	
	bool 	Init(LOG_LEVEL level, const char* logName, const char* logDir, const char* deleteDir);
	bool 	Start();
	bool 	Stop();
	void 	PrintThreadId();
	void 	DebugMsg(LOG_LEVEL level,const char* tag, const  char *format, ...);
	int 	GetDebugLevel(){return (int)_logLevel;}
	void 	SetDebugLevel(int level);
private:
	char* 	GetLogLevelName(LOG_LEVEL level);
	void 	WriteLogToFile();
	void 	addMessageToList(LOG_LEVEL log_level, const char* message, uint32_t message_len);
private:
	LOG_LEVEL 		_logLevel;
	char			_logName[MAX_LOGNAME_LENGTH];//without path
	char			_logDir[MAX_DIR_LENGTH];
	char			_logDeleteDir[MAX_DIR_LENGTH];
	char			_logFileName[MAX_LOGNAME_LENGTH];//with path
	
	FILE* 			fp;
	uint32_t	 	_logLines;

	CBaseThread*			_workerThread;
	uint32_t				m_pthreadId;

	int						_logMessageCount;
	CLock 					logQueueLock;		
	CCondition*  			logCondition;		
	log_message 			logMessageQueue[LOG_QUEUE_ARRAY][LOG_QUEUE_LENGTH];		
	uint32_t				next_free_idx[LOG_QUEUE_ARRAY];		
	uint32_t				active_queue;
};
#endif





