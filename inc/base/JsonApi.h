/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-28 10:59
** Email        : caseyguan@posteritytech.com
** Filename     : JsonApi.h
** Description  : 
** ******************************************************/
#ifndef _JSON_API_H_
#define _JSON_API_H_
#include <string>

using namespace std;

enum EValueType
{
	NullValueType = 0, //NULL value
	IntValueType,	//signed integer value
	UintValueType, //Unsigned integer value
	RealValueType, //Double value
	StringValueType, //UTF-8 string value
	BooleanValueType, //bool value
	ArrayValueType, //Bool value
	ObjectValueType //Object Value 
};

bool GetIntValue(const char* buffer, const char* name, int* value);
bool GetStringValue(const char* buffer, const char* name, string& value);


#endif
