#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HttpServer.h"

static int OnReceveRequest(char* url, char* data)
{
	printf("get request, url:%s, data:%s\n", url, data);
	return 0;
}

int main(int argc, char* argv[])
{
	//url postData
	//HttpServer IP port
	if(argc != 3)
	{
		printf("Please use HttpServer like this:\n");
		printf("HttpServer IP port\n");
		exit(0);
	}
	int port = atoi(argv[2]);
	printf("listen ip:%s,port:%d\n", argv[1], port);
	StartHttpServer(argv[1], port, OnReceveRequest, NULL);
	return 0;
}
