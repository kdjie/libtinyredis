//============================================================================
// Name        : RedisClient.cpp
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#include "RedisClient.h"

using namespace tinyredis;

CRedisClient::CRedisClient(const std::string& strIp, uint16_t uPort16, const std::string& strPass, uint32_t uDB, uint32_t uMiniSeconds)
: m_strIp(strIp)
, m_uPort16(uPort16)
, m_strPass(strPass)
, m_uDB(uDB)
, m_uMiniSeconds(uMiniSeconds)
, m_pRedisClient(NULL)
{
}

CRedisClient::~CRedisClient()
{
	__tryDisconnect();
}

const std::string& CRedisClient::getErrStr()
{
	return m_strErrStr;
}

redisReply* CRedisClient::command(const char* szFormat, ...)
{
	int32_t nRetryCount = 2;

	do
	{
		if (--nRetryCount <= 0)
			break;

		if (!__tryConnect())
			continue;

		va_list ap;
		va_start(ap, szFormat);

		CResult result(true);
		result = (redisReply*)redisvCommand(m_pRedisClient, szFormat, ap);

		va_end(ap);

		int32_t nRet = __commPrepareReply(result.get());

		if (nRet == -1)
			continue;
		else if (nRet == 1)
			break;

		__makeErrorString("");
		return result.release();

	} while (0);

	return NULL;
}

#ifdef __BLOCK__

// 执行原始命令，并返回结果
void CRedisClient::raw_commmand(const std::string& strRequest, std::string& strResponse)
{
	int32_t nRetryCount = 2;

	do
	{
		if (--nRetryCount <= 0)
			break;

		if (!__tryConnect())
			continue;

		if (myredisAppendCommand(m_pRedisClient, (char*)strRequest.data(), strRequest.size()) == REDIS_ERR)
		{
			strResponse = "- Out of memory \r\n";
			return;
		}

		CResult result(true);
		result = (redisReply*)myredisBlockForReply(m_pRedisClient);

		__makeResponse(result.get(), strResponse);
	} while (0);
}

// 发送管线请求
void CRedisClient::sendPipeRequest(const std::string& strRequest)
{
	int32_t nRetryCount = 2;

	do
	{
		if (--nRetryCount <= 0)
			break;

		if (!__tryConnect())
			continue;

		myredisAppendCommand(m_pRedisClient, (char*)strRequest.data(), strRequest.size());
	} while (0);
}

// 获取管线响应
void CRedisClient::getPipeResponse(std::string& strResponse)
{
	CResult result(true);
	result = (redisReply*)myredisBlockForReply(m_pRedisClient);

	__makeResponse(result.get(), strResponse);
}

#endif

bool CRedisClient::__tryConnect()
{
	if (m_pRedisClient)
		return true;

	// 连接

	struct timeval tvConnect;
	tvConnect.tv_sec = (m_uMiniSeconds/1000);
	tvConnect.tv_usec = (m_uMiniSeconds%1000)*1000;

	m_pRedisClient = redisConnectWithTimeout(m_strIp.c_str(), m_uPort16, tvConnect);

	if (m_pRedisClient == NULL || m_pRedisClient->err != REDIS_OK)
	{
		if (m_pRedisClient)
			__makeErrorString("redis[%s:%u] connect failed, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);
		else
			__makeErrorString("redis[%s:%u] connect failed", m_strIp.c_str(), m_uPort16);

		__tryDisconnect();
		return false;
	}

	// 认证

	if (m_strPass != "" && !__auth())
	{
		__tryDisconnect();
		return false;
	}

	// 切换DB

	if (m_uDB != 0 && !__selectDB())
	{
		__tryDisconnect();
		return false;
	}

	__makeErrorString("");
	return true;
}

void CRedisClient::__tryDisconnect()
{
	if (m_pRedisClient)
	{
		redisFree(m_pRedisClient);
		m_pRedisClient = NULL;
	}
}

bool CRedisClient::__auth()
{
	CResult result(true);
	result = (redisReply*)redisCommand(m_pRedisClient, "auth %s", m_strPass.c_str());

	if (!result)
		return false;

	if (!result.isStatus())
	{
		__makeErrorString("redis[%s:%u] auth ****** failed, not support", m_strIp.c_str(), m_uPort16);
		return false;
	}

	if (!result.isOK())
	{
		std::string strStatus;
		result.getString(strStatus);

		__makeErrorString("redis[%s:%u] auth ****** failed, response:%s", m_strIp.c_str(), m_uPort16, strStatus.c_str());
		return false;
	}

	__makeErrorString("");
	return true;
}

bool CRedisClient::__selectDB()
{
	CResult result(true);
	result = (redisReply*)redisCommand(m_pRedisClient, "select %d", m_uDB);

	if (!result)
		return false;

	if (!result.isStatus())
	{
		__makeErrorString("redis[%s:%u] select %d failed, not support", m_strIp.c_str(), m_uPort16, m_uDB);
		return false;
	}

	if (!result.isOK())
	{
		std::string strStatus;
		result.getString(strStatus);

		__makeErrorString("redis[%s:%u] auth %d failed, response:%s", m_strIp.c_str(), m_uPort16, m_uDB, strStatus.c_str());
		return false;
	}

	__makeErrorString("");
	return true;
}

int32_t CRedisClient::__commPrepareReply(redisReply* pReply)
{
	// 网络错误
	if (pReply == NULL || m_pRedisClient->err != REDIS_OK)
	{
		__makeErrorString("redis[%s:%u] net error, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);

		__tryDisconnect();
		return -1; 
	}

	// 指令错误
	if (pReply->type == REDIS_REPLY_ERROR)
	{
		__makeErrorString("redis[%s:%u] command error, desc:%s", m_strIp.c_str(), m_uPort16, m_pRedisClient->errstr);

		return 1; 
	}

	return 0;
}

void CRedisClient::__makeResponse(redisReply* pReply, std::string& strResponse)
{
	if (pReply == NULL || m_pRedisClient->err != REDIS_OK)
	{
		__tryDisconnect();
		strResponse = "- Connect failed \r\n";
		return;
	}

	// 根据不同的返回类型，组织返回语句
	strResponse = "";

	std::string strCRLF = "\r\n";
	std::string strPre;
	std::string strFLine;

	if (pReply->type == REDIS_REPLY_ERROR)
	{
		strPre = "-";
		strFLine.assign(pReply->str, pReply->len);

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;
	}
	else if (pReply->type == REDIS_REPLY_STATUS)
	{
		strPre = "+";
		strFLine.assign(pReply->str, pReply->len);

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;
	}
	else if (pReply->type == REDIS_REPLY_NIL)
	{
		strPre = "$";
		strFLine = "-1";

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;
	}
	else if (pReply->type == REDIS_REPLY_INTEGER)
	{
		strPre = ":";
		strFLine = convert<std::string>(pReply->integer);;

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;
	}
	else if (pReply->type == REDIS_REPLY_STRING)
	{
		strPre = "$";
		strFLine = convert<std::string>(pReply->len);

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;

		strResponse += std::string(pReply->str, pReply->len);
		strResponse += strCRLF;
	}
	else if (pReply->type == REDIS_REPLY_ARRAY)
	{
		strPre = "*";
		strFLine = convert<std::string>(pReply->elements);

		strResponse += strPre;
		strResponse += strFLine;
		strResponse += strCRLF;

		for (size_t i = 0; i < pReply->elements; ++i)
		{
			redisReply* pSubReply = pReply->element[i];
			if (pSubReply->type == REDIS_REPLY_STRING)
			{
				strResponse += "$";
				strResponse += convert<std::string>(pSubReply->len);
				strResponse += strCRLF;
				strResponse += std::string(pSubReply->str, pSubReply->len);
				strResponse += strCRLF;
			}
			else if (pSubReply->type == REDIS_REPLY_NIL)
			{
				strResponse += "$";
				strResponse += "-1";
				strResponse += strCRLF;
			}
		}
	}
}

void CRedisClient::__makeErrorString(const char* szFormat, ...)
{
	char szErrTmp[1024] = {0};

	va_list ap;
	va_start(ap, szFormat);
	vsnprintf(szErrTmp, 1024, szFormat, ap);
	va_end(ap);

	m_strErrStr = szErrTmp;
}


CResult::CResult(bool bAutoFree)
: m_pReply(NULL)
, m_bAutoFree(bAutoFree)
{
}
CResult::~CResult()
{
	if (m_bAutoFree)
		free();
}

void CResult::free()
{
	if (m_pReply)
	{
		freeReplyObject(m_pReply);
		m_pReply = NULL;
	}
}

redisReply* CResult::get()
{
	return m_pReply;
}

redisReply* CResult::release()
{
	redisReply* pRet = m_pReply;
	m_pReply = NULL;

	return pRet;
}

bool CResult::isArray()
{
	return (m_pReply->type == REDIS_REPLY_ARRAY);
}

bool CResult::isInteger()
{
	return (m_pReply->type == REDIS_REPLY_INTEGER);
}

bool CResult::isString()
{
	return (m_pReply->type == REDIS_REPLY_STRING);
}

bool CResult::isNil()
{
	return (m_pReply->type == REDIS_REPLY_NIL);
}

bool CResult::isStatus()
{
	return (m_pReply->type == REDIS_REPLY_STATUS);
}

redisReply* CResult::getSubReply(size_t uPos)
{
	return m_pReply->element[uPos];
}

size_t CResult::getArraySize()
{
	return m_pReply->elements;
}

int64_t CResult::getInteger()
{
	return m_pReply->integer;
}

void CResult::getString(std::string& str)
{
	str.assign(m_pReply->str, m_pReply->len);
}

bool CResult::isOK()
{
	std::string str;
	getString(str);
	return (str == "OK");
}

bool CResult::operator ! ()
{
	return (m_pReply == NULL);
}

void CResult::operator = (redisReply *pReply)
{
	free();
	
	m_pReply = pReply;
}
