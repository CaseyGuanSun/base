#include "Debug.h"
extern "C"{
#include "ipcsock.h"
}

#define TAG "Debug"

typedef struct LogLevelName
{		
	LOG_LEVEL level;		
	char  	  name[128];
}LogLevelName;

static LogLevelName GLogLevelName[] = {		
			{LOG_LEVEL_NON, 	"NOLOG"},		
			{LOG_LEVEL_ERROR, 	"ERROR"},		
			{LOG_LEVEL_WARNING, "WARING"},		
			{LOG_LEVEL_INFO, 	"INFO"},		
			{LOG_LEVEL_DEBUG, 	"DEBUG"},		
			{LOG_LEVEL_VERBOSE, "VERBOSE"}
};

CDebug* CDebug::Create(LOG_LEVEL level, const char* logName, const char* logDir, const char* deleteDir)
{
	bool res = false;
	
	CDebug* ptr = new CDebug();
	if(ptr == NULL)
	{
		return NULL;
	}
	
	res = ptr->Init(level, logName, logDir, deleteDir);
	if(!res)
	{
		delete ptr;
		return NULL;
	}

	res = ptr->Start();
	if(!res)
	{
		delete ptr;
		return NULL;
	}
	
	return ptr;
}

CDebug::CDebug():
	_logLevel(LOG_LEVEL_NON)
   ,fp(NULL)
   ,_logLines(0)
   ,_logMessageCount(0)
{
	logCondition = new CCondition(&logQueueLock);
}

CDebug::~CDebug()
{
	if(_workerThread)
	{
		delete _workerThread;
		_workerThread = NULL;
	}
	if(logCondition)
	{
		delete logCondition;
	}
}

bool CDebug::CbThread(void* context)
{
	if(NULL == context)
	{
		return false;
	}
	return reinterpret_cast<CDebug*>(context)->CbRunImpl();
}

bool CDebug::CbRunImpl()
{
	{
	//TODO:set _logMessageCount read after write over
	//	CCriticalSectionScoped scope(&logQueueLock);
		logQueueLock.Lock();
		while(_logMessageCount<=0)
		{
			if ( false == logCondition->WaitTime(500))
			{
				//file flush
				if (fp)
				{
					fflush(fp);
				}
				continue;
			}
		}
		logQueueLock.Unlock();
	}
	//There are new log message
	WriteLogToFile();
	return true;
}

void 	CDebug::WriteLogToFile()
{
	//write the log queue message to file	
	uint32_t local_active_queue = 0;	
	uint32_t local_free_idx = 0;	
	log_message* logMessage;	
	int tmp_level =0;	
	int ret;
	
	//Two Buffers, one for writing message to file and one for storing new message.	
	{
		CriticalSectionScoped scope(&logQueueLock);
		local_active_queue = active_queue;	
		local_free_idx = next_free_idx[active_queue];	
		next_free_idx[active_queue] = 0;		
		if(active_queue ==0)	
		{		
			active_queue =1;	
		}else{		
			active_queue = 0;	
		}
	}
	
	if (local_free_idx == 0)
	{		
		return;
	}

	int i=0; 
	for (i=0; i<local_free_idx; i++)
	{
		logMessage = &logMessageQueue[local_active_queue][i];
		if (fp && logMessage->message_len > 0)
		{
			if (_logLines >= MAX_LOG_LINES)
			{
				char 	tmp[MAX_LOGNAME_LENGTH];
				char 	time_buf[64];
				memset(tmp, 0, sizeof(tmp));
				memset(time_buf, 0, sizeof(time_buf));

				snprintf(tmp,sizeof(tmp)-1, "%s/%s_%s", _logDeleteDir, _logName, ansiTimeToString(time(0),"MMDDHHMI",time_buf));

				if(fp != NULL)
				{
					fflush(fp);
					fclose(fp);
				}
				fp =NULL;
				ret = rename(_logFileName, tmp);
				if(ret == -1)
				{
					return;
				}
				fp = fopen(_logFileName, "a+");
				if(fp == NULL)
				{
					return;
				}
				_logLines = 0;
			}

			if (_logLines == 0)
			{
				//TODO:Write the log header
				char message[256];
				char dateBuf[128];
				int len =0 ;
				memset(message, 0, sizeof(message));
				memset(dateBuf, 0, sizeof(dateBuf));
				sprintf(message, "Date:%s\n", getCurrentTimeString(dateBuf));
				if (fp)
				{
					len = strlen(message);
					fwrite(message, sizeof(char), len, fp);
				}
			}

			if (fp)
			{
				fwrite(logMessage->message_buf, sizeof(char), logMessage->message_len, fp);
				_logLines++;
				_logMessageCount--;
			}
			memset(logMessage->message_buf, 0x00, sizeof(logMessage->message_buf));
			logMessage->message_len = 0;
		}
	}
	if(fp)
	{
		fflush(fp);
	}
}


bool CDebug::Init(LOG_LEVEL level, const char* logName, const char* logDir, const char* deleteDir)
{
	if(NULL == logName ||
	   NULL == logDir  ||
	   NULL == deleteDir)
	{
		return false;
	}

	if(level < 0)
	{
		_logLevel = LOG_LEVEL_NON;
	}else if(level > 5)		
	{				
		_logLevel = LOG_LEVEL_VERBOSE;		
	}else
	{
		_logLevel = (LOG_LEVEL)level;
	}

	memset(_logName, 0x00, sizeof(_logName));
	snprintf(_logName, sizeof(_logName)-1, "%s", logName);
	memset(_logDir, 0x00, sizeof(_logDir));
	snprintf(_logDir, sizeof(_logDir)-1, "%s", logDir);
	memset(_logDeleteDir, 0x00, sizeof(_logDeleteDir));
	snprintf(_logDeleteDir, sizeof(_logDeleteDir)-1, "%s", deleteDir);
	memset(_logFileName, 0x00, sizeof(_logFileName));
	snprintf(_logFileName, sizeof(_logFileName)-1, "%s/%s", _logDir, _logName);

	fp = fopen(_logFileName, "a+");
	if(fp == NULL)
	{
		return false;
	}

	int i=0;
	int j=0;
	for (i=0; i<LOG_QUEUE_ARRAY; i++)		
	{			
		for (j=0; j<LOG_QUEUE_LENGTH; j++)			
		{	
			memset(logMessageQueue[i][j].message_buf, 0x00, sizeof(logMessageQueue[i][j].message_buf));
			logMessageQueue[i][j].message_len = 0;			
		}			
		next_free_idx[i]=0;		
	}		
	active_queue = 0;

	return true;
}

bool CDebug::Start()
{
	_workerThread = CBaseThread::Create(CbThread, this, PRIORITY_NORMAL, "LogThread");
	if(_workerThread == NULL)
	{
		return false;
	}
	if(!_workerThread->Start(m_pthreadId))
	{
		return false;
	}
	return true;
}

bool CDebug::Stop()
{
	int i=0;
	int j=0;
	log_message* logMessage = NULL;

	if(NULL == _workerThread)
	{
		return false;
	}
		
	if(!_workerThread->Stop())
	{
		return false;
	}
	delete _workerThread;

	//Write the remain log in the message queue
	for (i=0; i<LOG_QUEUE_ARRAY; i++)
	{
		if (next_free_idx[i] > 0)
		{
			for(j=0; j<next_free_idx[i]; j++)
			{
				logMessage = &logMessageQueue[i][j];
				if (fp)
				{
					fwrite(logMessage->message_buf, sizeof(char),logMessage->message_len , fp);
					memset(logMessage->message_buf, 0x00, sizeof(logMessage->message_buf));
					logMessage->message_len = 0;
				}
			}
			next_free_idx[i]=0;
		}
	}

	if(fp)
	{
		fflush(fp);
		fclose(fp);
		fp = NULL;
	}

	return true;
}


void CDebug::DebugMsg(LOG_LEVEL level,const char* tag, const  char *format, ...)
{
	if(NULL == tag )		
	{				
		return;		
	}
	
	if(_logLevel <= 0)		
	{				
		return;		
	}
	if(level > 5)		
	{				
		level = LOG_LEVEL_VERBOSE;		
	}
	
	if(level > _logLevel)		
	{				
		return;		
	}

	char   msgBuf[MAX_LOG_BUF_LENGTH];		
	memset(msgBuf, 0x00, sizeof(msgBuf));		
	va_list arg_ptr;		
	va_start(arg_ptr, format);		
	vsprintf(msgBuf, format, arg_ptr);		
	va_end(arg_ptr);

	char	buffer[MAX_LOG_BUF_LENGTH];
	char	time_buf[128];
	snprintf(buffer, sizeof(buffer)-1, "[%s] [%s] [%s]:%s\n", 
		GetLogLevelName((LOG_LEVEL)level), getCurrentTimeString(time_buf), 
		tag, msgBuf);
	buffer[sizeof(buffer)-1] = '\0';
	addMessageToList((LOG_LEVEL)level, buffer, strlen(buffer));
}

void CDebug::SetDebugLevel(int level)
{
	if(level < 0)
	{
		_logLevel = LOG_LEVEL_NON;
	}else if(level > LOG_LEVEL_VERBOSE)
	{
		_logLevel = LOG_LEVEL_VERBOSE;
	}else
	{
		_logLevel = (LOG_LEVEL)level;	
	}
}

void CDebug::addMessageToList(LOG_LEVEL log_level, const char* message, uint32_t message_len)
{	
	CriticalSectionScoped scope(&logQueueLock);
	if(next_free_idx[active_queue] >= LOG_QUEUE_LENGTH)	
	{		
		//The queue is full		
		return;	
	}	
	uint32_t idx = next_free_idx[active_queue];	
	next_free_idx[active_queue]++;	
	logMessageQueue[active_queue][idx].log_level = log_level;	
	logMessageQueue[active_queue][idx].message_len = message_len;	
	strncpy(logMessageQueue[active_queue][idx].message_buf, message, message_len);	
	_logMessageCount++;
	if(next_free_idx[active_queue] == LOG_QUEUE_LENGTH -1)	
	{		
		//Log message is more than can be worked off. now print a warning		
		const char waring_message[] = "WARNING:The debug message is more than can be worked off\n";		
		logMessageQueue[active_queue][next_free_idx[active_queue]].log_level = LOG_LEVEL_WARNING;		
		logMessageQueue[active_queue][next_free_idx[active_queue]].message_len = strlen(waring_message);		
		strncpy(logMessageQueue[active_queue][next_free_idx[active_queue]].message_buf, waring_message, 
			strlen(waring_message));		
		next_free_idx[active_queue]++;	
		_logMessageCount++;
	}	
	logCondition->Notify();
}

char* CDebug::GetLogLevelName(LOG_LEVEL level)
{		
	return GLogLevelName[level].name;
}

void CDebug::PrintThreadId()
{
	//_workerThread->PrintThreadId();
}



