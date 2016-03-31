/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-22 15:12
** Email        : caseyguan@posteritytech.com
** Filename     : RequestParam.h
** Description  : 
** ******************************************************/
#ifndef _REQUEST_PARAM_H_
#define _REQUEST_PARAM_H_
#include <string>

using namespace std;

typedef enum {
	DATA_TYPE_STRING,
	DATE_TYPE_INTEGER,
	DATE_TYPE_FLOAT
}RequestDataType;

typedef struct RequestParam
{
	string 				RequestName;
	RequestDataType 	dataType;
	union{
		char 	strData[512];
		int 	iData;
		float   fData;
		}RequestData;
}RequestParam;

#endif


