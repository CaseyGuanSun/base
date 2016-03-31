/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-22 23:04
** Email        : caseyguan@posteritytech.com
** Filename     : HttpServer.c
** Description  : 
** ******************************************************/
#include "HttpServer.h"
#include <event.h>
#include <evhttp.h>

static OnReceiveRequest OnReceiveRequestCallback;
static PrintLog  debugFun = NULL;
static struct event_base * base = NULL;
static struct evhttp * http_server = NULL; 

void httpd_handler(struct evhttp_request *req, void *arg)
{
	const char* uri;
	int ret= -1;
	char* decoded_uri;
	char* post_data;

	if(debugFun)
	{
		char tmp[256];
		sprintf(tmp, "%s", "recevie http request");
		debugFun(tmp, strlen(tmp));
	}
	uri = evhttp_request_uri(req);
	if(NULL == uri)
	{
		if(debugFun)
		{
			char tmp[256];
			sprintf(tmp, "%s", "Error uril is NULL");
			debugFun(tmp, strlen(tmp));
		}
		return ;
	}
	decoded_uri = evhttp_decode_uri(uri);
	if(NULL == decoded_uri)
	{
		if(debugFun)
		{
			char tmp[256];
			sprintf(tmp, "%s", "Error decoded_uri is NULL");
			debugFun(tmp, strlen(tmp));
		}
		return;
	}
	post_data = (char*)EVBUFFER_DATA(req->input_buffer);
	if(NULL == post_data)
	{
		if(debugFun)
		{
			char tmp[256];
			sprintf(tmp, "%s", "Error post_data is NULL");
			debugFun(tmp, strlen(tmp));
		}
		return;
	}
	if(debugFun)
	{
		char tmp[1024];
		sprintf(tmp, "!!!uri:%s, data:%s", decoded_uri, post_data);
		debugFun(tmp, strlen(tmp));
	}

	if(OnReceiveRequestCallback)
	{
		ret = OnReceiveRequestCallback(decoded_uri, post_data);
	}
	struct evbuffer *buf = evbuffer_new();
	if(NULL == buf)
	{
		if(debugFun)
		{
			char tmp[256];
			sprintf(tmp, "%s", "Error buf is NULL");
			debugFun(tmp, strlen(tmp));
		}
		return ;
	}
	
	if(ret == 0)
	{
		evhttp_send_reply(req, HTTP_OK, "OK", buf);
	}else
	{
		evhttp_send_reply(req, HTTP_OK, "FALSE", buf);
	}
	evbuffer_free(buf);
}

int StartHttpServer(const char* listenIp, unsigned int listenPort, OnReceiveRequest callback, PrintLog logCallback)
{
	int ret;

	OnReceiveRequestCallback = callback;
	debugFun = logCallback;

    base = event_base_new();
    http_server = evhttp_new(base);
    if(!http_server)
	{
	    return -1;
    }
	if(listenIp == NULL)
	{
		ret = evhttp_bind_socket(http_server, "0.0.0.0", listenPort);
	}else
	{
		ret = evhttp_bind_socket(http_server, listenIp, listenPort);
	}
    
    if(ret!=0)
    {
		return -2;
    }
	if(debugFun)
	{
		char tmp[256];
		sprintf(tmp, "%s", "event will dispatch");
		debugFun(tmp, strlen(tmp));
	}
	evhttp_set_gencb(http_server, httpd_handler, NULL);
    event_base_dispatch(base);

    evhttp_free(http_server);
	return 0;
}

