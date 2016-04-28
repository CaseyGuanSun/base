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

char* image_url = "https://passport.baidu.com/cgi-bin/genimage?njIcaptchaservice353139333671536b78326145756a6e552b3146645a6e584a344b6963716f534a6469494a4f6a7a413337325a595278554d4b4d41626642502f4e507046645931714f61657a64554e4531573241654165364d4c3639516941343076676f4e2b46325a545956775443646f74566539347178454d7841686f4766736e374a566d31656477536a6c4570666c4d57577445796d463439776c4531346c41685970394a644c6d386a6438576a756d732f42654f767956487869386847524e454c767a6a676d5a675a617a666f5776425533744742356f4153724a71596254394a48755541316271436c3130794f3145645a5561592b5a6a70386e674b51764a614573526471356764417a466277724f58496368716d5865626a304d555954337437693147536f346973412f4873766f5341&v=1461858654084";

//HttpClientTest GET_FILE url  filePath
int main(int argc, char* argv[])
{
	char* command = NULL;
	char* url = NULL;
	char* filePath = NULL;
	int ret = 0;

	/*
	if(argc != 4)
	{
		printf("Please use like this, argc:%d:\n", argc);
		printf("HttpClientTest GET_FILE url filePath\n");
		return 0;
	}
	*/

	ret = HttpGetFile(image_url, "./test.jpg");
/*
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
	*/
	return 0;	
}

