/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-04-28 22:27
** Email        : caseyguan@posteritytech.com
** Filename     : HttpClientTest.c
** Description  : 
** ******************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HttpClient.h"

#define GET_FILE "GET_FILE"

//HttpClientTest GET_FILE url  filePath
int main(int argc, char* argv[])
{
	char* command = NULL;
	char* url = NULL;
	char* filePath = NULL;
	int ret = 0;
	if(argc != 4)
	{
		printf("Please use like this, argc:%d:\n", argc);
		printf("HttpClientTest GET_FILE url filePath\n");
		return 0;
	}

	command = argv[1];
	url = argv[2];
	filePath = argv[3];

	if(strcmp(command, GET_FILE))
	{
		ret = HttpGetFile(url, filePath);	
	}else
	{
		printf("Command:%s is not supported\n", command);
	}
	return 0;	
}

