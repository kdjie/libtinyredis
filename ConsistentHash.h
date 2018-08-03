//============================================================================
// Name        : ConsistentHash.h
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#pragma once

#include <stdint.h>
#include <string.h>
#include <openssl/md5.h>

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <set>

#define DEFAULTNUMBEROFREPLICAS 200

enum EHashFuntcionType
{
	ENUM_FUNC_MD5 = 0,
	ENUM_FUNC_NEWHASH,
	ENUM_FUNC_FNVHASH
};

class CHashFunction
{
public:
	static uint32_t md5Hash(const char *pData, size_t uSize)
	{
		unsigned char szMD5[16] = {0};
		MD5((const unsigned char *)pData, uSize, szMD5);

		uint32_t uHash = 
			  ( ((uint32_t)(szMD5[3] & 0xFF)) << 24 )
			| ( ((uint32_t)(szMD5[2] & 0xFF)) << 16 )
			| ( ((uint32_t)(szMD5[1] & 0xFF)) << 8 )
			|   ((uint32_t)(szMD5[0] & 0xFF));

		return uHash;
	}

	static uint32_t newHash(const char *pData, size_t uSize)
	{
		uint32_t uHash = 0;

		for (size_t i = 0; i < uSize; ++i)
		{
			uHash *= 16777619;
			uHash ^= (uint32_t) *((unsigned char *)(pData+i));
		}

		return uHash;
	}

	static uint32_t newHash(uint32_t u32)
	{
		return newHash((const char *) &u32, sizeof(u32));
	}

	static uint32_t fnvHash(const char *pData, size_t uSize)
	{
		register uint32_t uMagic = 16777619;
		register uint32_t uHash = 0x811C9DC5;//2166136261L;

		for (size_t i = 0; i < uSize; ++i)
		{
			uHash = (uHash ^ (*(unsigned char *)(pData+i))) * uMagic;
		}

		uHash += uHash << 13;
		uHash ^= uHash >> 7;
		uHash += uHash << 3;
		uHash ^= uHash >> 17;
		uHash += uHash << 5;

		return uHash;    
	}

	static uint32_t fnvHash(uint32_t u32)
	{
		return fnvHash((const char *) &u32, sizeof(u32));
	}
};

class CConsistentHash
{
public:
	typedef std::map<uint32_t, uint32_t> MAP_HASH_CIRCLE;

	CConsistentHash(MAP_HASH_CIRCLE& m_mapHashCircle, EHashFuntcionType emFuncType = ENUM_FUNC_MD5, uint32_t numberOfReplicas = DEFAULTNUMBEROFREPLICAS)
		: m_mapHashCircle(m_mapHashCircle)
		, m_emFuncType(emFuncType)
		, m_numberOfReplicas(numberOfReplicas)
	{
	}

	void addHash(uint32_t uValue)
	{
		for (uint32_t i = 0; i < m_numberOfReplicas; ++i) 
		{
			std::string strHash = __toHashString(uValue, i);

			uint32_t uHashKey = 0;

			switch (m_emFuncType)
			{
			case ENUM_FUNC_MD5:
				{
					uHashKey = CHashFunction::md5Hash(strHash.data(), strHash.size());
				}
				break;
			case ENUM_FUNC_NEWHASH:
				{
					uHashKey = CHashFunction::newHash(strHash.data(), strHash.size());
				}
				break;
			case ENUM_FUNC_FNVHASH:
				{
					uHashKey = CHashFunction::fnvHash(strHash.data(), strHash.size());
				}
				break;
			}

			m_mapHashCircle[uHashKey] = uValue;
		}
	}

	void removeHash(uint32_t uValue)
	{
		for (uint32_t i = 0; i < m_numberOfReplicas; ++i) 
		{
			std::string strHash = __toHashString(uValue, i);

			uint32_t uHashKey = 0;

			switch (m_emFuncType)
			{
			case ENUM_FUNC_MD5:
				{
					uHashKey = CHashFunction::md5Hash(strHash.data(), strHash.size());
				}
				break;
			case ENUM_FUNC_NEWHASH:
				{
					uHashKey = CHashFunction::newHash(strHash.data(), strHash.size());
				}
				break;
			case ENUM_FUNC_FNVHASH:
				{
					uHashKey = CHashFunction::fnvHash(strHash.data(), strHash.size());
				}
				break;
			}

			m_mapHashCircle.erase(uHashKey);
		}
	}

	uint32_t getHashValue(uint32_t uKey)
	{
		if (m_mapHashCircle.empty())
			return -1;

		std::string strKey = __toHashString(uKey);

		uint32_t uHashKey = 0;

		switch (m_emFuncType)
		{
		case ENUM_FUNC_MD5:
			{
				uHashKey = CHashFunction::md5Hash(strKey.data(), strKey.size());
			}
			break;
		case ENUM_FUNC_NEWHASH:
			{
				uHashKey = CHashFunction::newHash(strKey.data(), strKey.size());
			}
			break;
		case ENUM_FUNC_FNVHASH:
			{
				uHashKey = CHashFunction::fnvHash(strKey.data(), strKey.size());
			}
			break;
		}

		MAP_HASH_CIRCLE::iterator iter = m_mapHashCircle.lower_bound(uHashKey);
		if (iter != m_mapHashCircle.end())
		{
			return iter->second;
		}

		return (m_mapHashCircle.begin())->second;
	}

private:

	static std::string __toHashString(uint32_t uValue, uint32_t uIndex)
	{
		std::stringstream ssHash;
		ssHash << "KDJ::HASH VALUE:" << uValue << " INDEX:" << uIndex;
		return ssHash.str();
	}

	static std::string __toHashString(uint32_t uKey)
	{
		std::stringstream ssKey;
		ssKey << "KDJ::HASH KEY:" << uKey;
		return ssKey.str();
	}

private:
	MAP_HASH_CIRCLE& m_mapHashCircle;
	EHashFuntcionType m_emFuncType;
	uint32_t m_numberOfReplicas;
};
