//============================================================================
// Name        : RedisClient.h
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#pragma once

#include <stdint.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include <hiredis/hiredis.h>

namespace tinyredis
{

	class CRedisClient
	{
	public:
		CRedisClient(const std::string& strIp, uint16_t uPort16, const std::string& strPass, uint32_t uMiniSeconds);
		virtual ~CRedisClient();

		const std::string& getErrStr();

		redisReply* command(const char* szFormat, ...);

#ifdef __BLOCK__
		// 执行原始命令，并返回结果
		void raw_commmand(const std::string& strRequest, std::string& strResponse);

		// 发送管线请求
		void sendPipeRequest(const std::string& strRequest);

		// 获取管线响应
		void getPipeResponse(std::string& strResponse);
#endif

	private:

		bool __tryConnect();
		void __tryDisconnect();
		bool __auth();

		int32_t __commPrepareReply(redisReply* pReply);

		void __makeResponse(redisReply* pReply, std::string& strResponse);

		void __makeErrorString(const char* szFormat, ...);

	private:
		std::string m_strIp;
		uint16_t m_uPort16;
		std::string m_strPass;
		uint32_t m_uMiniSeconds;

		redisContext* m_pRedisClient;

		std::string m_strErrStr;
	};

	class CResult
	{
	private:
		redisReply *m_pReply;
		bool m_bAutoFree;

	public:
		CResult(bool bAutoFree = true);
		virtual ~CResult();
		
		void free();
		redisReply* get();
		redisReply* release();

		bool isArray();
		bool isInteger();
		bool isString();
		bool isNil();
		bool isStatus();

		redisReply* getSubReply(size_t uPos);

		size_t getArraySize();
		int64_t getInteger();
		void getString(std::string& str);
		bool isOK();

		bool operator ! ();
		void operator = (redisReply *pReply);
	};

	template <typename R, typename T>
	static R convert(const T& t)
	{
		R r;

		std::stringstream ss;
		ss << t;
		ss >> r;

		return r;
	}

	class CHashResult
	{
	public:

		void addKV(const std::string& K, const std::string& V)
		{
			m_mapKV[K] = V;
		}

		template <typename R>
		R getValue(const std::string& K, const R& def)
		{
			R r = def;

			std::map<std::string, std::string>::iterator iter = m_mapKV.find(K);
			if (iter == m_mapKV.end())
				return r;

			std::stringstream ss;
			ss << iter->second;
			ss >> r;
			return r;
		}

		std::string getValue(const std::string& K, const std::string& def)
		{
			std::string r = def;

			std::map<std::string, std::string>::iterator iter = m_mapKV.find(K);
			if (iter == m_mapKV.end())
				return r;

			return iter->second;
		}

	private:
		std::map<std::string, std::string> m_mapKV;
	};

}
