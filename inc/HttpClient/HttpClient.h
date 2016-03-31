/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-23 00:40
** Email        : caseyguan@posteritytech.com
** Filename     : HttpClient.h
** Description  : 
** ******************************************************/
#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

int HttpPost(const char* strUrl, const char* strPost, char* strResponse);
int HttpGet(const char* strUrl, char* strResponse);


#endif

