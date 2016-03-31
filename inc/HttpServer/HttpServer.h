/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-22 23:04
** Email        : caseyguan@posteritytech.com
** Filename     : HttpServer.h
** Description  : 
** ******************************************************/
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

typedef int (*OnReceiveRequest)(char* queryUrl, char* data);
typedef int (*PrintLog)(char* msg, int len);
int StartHttpServer(const char* listenIp, unsigned int listenPort, OnReceiveRequest callback, PrintLog logCallback);

#endif


