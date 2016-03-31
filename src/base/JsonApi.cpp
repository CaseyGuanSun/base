/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-28 11:00
** Email        : caseyguan@posteritytech.com
** Filename     : JsonApi.cpp
** Description  : 
** ******************************************************/
#include "JsonApi.h"
#include "json/json.h"

using namespace Json;

bool GetIntValue(const char* buffer, const char* name, int* value)
{
	Reader jsonRader;
	Value jsonValue;

	if(NULL == buffer)
	{
		return false;
	}

	if (jsonRader.parse(buffer, jsonValue))
    {  
        *value = jsonValue[name].asInt();
		return true;
    } 
	return false;
}
bool GetStringValue(const char* buffer, const char* name, string& value)
{
	Reader jsonRader;
	Value jsonValue;

	if(NULL == buffer)
	{
		return false;
	}

	if (jsonRader.parse(buffer, jsonValue))
    {  
        value = jsonValue[name].asString();
		return true;
    } 
	return false;
}

