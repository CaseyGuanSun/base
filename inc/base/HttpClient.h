/* ******************************************************
** Author       : CaseyGuan
** Last modified: 2016-03-17 17:58
** Email        : caseyguan@posteritytech.com
** Filename     : HttpClient.h
** Description  : Define Http Client Class
** ******************************************************/
#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_
#include <string>
#include "ostype.h"

using namespace std;

class CHttpClient
{
public:
	CHttpClient(void);
	~CHttpClient(void);
	
public:
	/**
	* @brief HTTP POST����
	* @param strUrl �������,�����Url��ַ,��:http://www.baidu.com
	* @param strPost �������,ʹ�����¸�ʽpara1=val1?2=val2&��
	* @param strResponse �������,���ص�����
	* @return �����Ƿ�Post�ɹ�
	*/
	int Post(const std::string & strUrl, const std::string & strPost, std::string & strResponse);

	/**
	* @brief HTTP GET����
	* @param strUrl �������,�����Url��ַ,��:http://www.baidu.com
	* @param strResponse �������,���ص�����
	* @return �����Ƿ�Post�ɹ�
	*/
	int Get(const std::string & strUrl, std::string & strResponse);

	/**
	* @brief HTTPS POST����,��֤��汾
	* @param strUrl �������,�����Url��ַ,��:https://www.alipay.com
	* @param strPost �������,ʹ�����¸�ʽpara1=val1?2=val2&��
	* @param strResponse �������,���ص�����
	* @param pCaPath �������,ΪCA֤���·��.�������ΪNULL,����֤��������֤�����Ч��.
	* @return �����Ƿ�Post�ɹ�
	*/
	int Posts(const std::string & strUrl, const std::string & strPost, std::string & strResponse, const char * pCaPath = NULL);

	/**
	* @brief HTTPS GET����,��֤��汾
	* @param strUrl �������,�����Url��ַ,��:https://www.alipay.com
	* @param strResponse �������,���ص�����
	* @param pCaPath �������,ΪCA֤���·��.�������ΪNULL,����֤��������֤�����Ч��.
	* @return �����Ƿ�Post�ɹ�
	*/
	int Gets(const std::string & strUrl, std::string & strResponse, const char * pCaPath = NULL);
	
public:
	void SetDebug(bool bDebug);
	
	string UrlEncode(string& szToEncode);

private:
	bool m_bDebug;

};

#endif


