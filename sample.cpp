//============================================================================
// Name        : sample.cpp
// Author      : kdjie
// Version     : 1.0
// Copyright   : @2015
// Description : 14166097@qq.com
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

#include "RedisFactory.h"

int main(int argc, char* argv[])
{
	using namespace tinyredis;

	// 创建工厂实例
	CRedisFactory redisFactory;

	// 初使化redis服务配置
	redisFactory.addRedis("127.0.0.1", 3000, "123456", 0, 1000);
	redisFactory.addRedis("127.0.0.1", 3001, "123456", 0, 1000);

	// 简单的，单KV读写
	do
	{
		uint32_t uId = 1;
		std::string strName = "zhang3";

		CRedisClient* pRedis = redisFactory.getRedis(uId);
		CResult result(true);
		result = pRedis->command("set name:%u %s", uId, strName.c_str());
		if (!result)
		{
			printf("set failed : %s \n", pRedis->getErrStr().c_str());
			break;
		}

		printf("set ok \n");

		result = pRedis->command("get name:%u", uId);
		if (!result)
		{
			printf("get failed : %s \n", pRedis->getErrStr().c_str());
			break;
		}

		if (result.isNil())
		{
			printf("not found! \n");
			break;
		}

		if (result.isString())
		{
			std::string strValue;
			result.getString(strValue);
			printf("get value : %s \n", strValue.c_str());
		}
	} while (0);

	printf("\n");

	// 来点复杂的，多KV读写
	do 
	{
		std::vector<uint32_t> vecId;
		vecId.push_back(1);
		vecId.push_back(2);
		vecId.push_back(3);
		
		std::vector<std::string> vecName;
		vecName.push_back("zhang3");
		vecName.push_back("li4");
		vecName.push_back("wang5");

		// 计算Key的分布
		std::map<CRedisClient*, std::vector<std::pair<uint32_t, std::string> > > mapRedisPair;
		for (uint32_t i = 0; i < vecId.size(); ++i)
		{
			CRedisClient* pRedis = redisFactory.getRedis( vecId[i] );
			mapRedisPair[pRedis].push_back( std::make_pair(vecId[i], vecName[i]) );
		}

		// 逐redis批量读写
		for (std::map<CRedisClient*, std::vector<std::pair<uint32_t, std::string> > >::iterator iterRedis = mapRedisPair.begin(); 
			iterRedis != mapRedisPair.end(); ++iterRedis)
		{
			CRedisClient* pRedis = iterRedis->first;
			std::vector<std::pair<uint32_t, std::string> > vecPair = iterRedis->second;

			// 拼装命令
			std::stringstream ssCmd;
			ssCmd << "mset";
			for (std::vector<std::pair<uint32_t, std::string> >::iterator iterPair = vecPair.begin(); iterPair != vecPair.end(); ++iterPair)
			{
				ssCmd << " " << iterPair->first << " " << iterPair->second;
			}

			CResult result(true);
			result = pRedis->command(ssCmd.str().c_str());
			if (!result)
			{
				printf("mset failed : %s \n", pRedis->getErrStr().c_str());
				break;
			}

			printf("mset ok \n");

			// 拼装命令
			ssCmd.str("");
			ssCmd << "mget";
			for (std::vector<std::pair<uint32_t, std::string> >::iterator iterPair = vecPair.begin(); iterPair != vecPair.end(); ++iterPair)
			{
				ssCmd << " " << iterPair->first;
			}

			result = pRedis->command(ssCmd.str().c_str());
			if (!result)
			{
				printf("mget failed : %s \n", pRedis->getErrStr().c_str());
				break;
			}

			if (!result.isArray())
			{
				printf("result not array \n");
				break;
			}

			for (size_t i = 0; i < result.getArraySize(); ++i)
			{
				CResult subResult(false);
				subResult = result.getSubReply(i);

				if (subResult.isNil())
				{
					printf("uid:%d not found \n", vecPair[i].first);
				}
				else
				{
					std::string strValue;
					subResult.getString(strValue);
					printf("uid:%d name : %s \n", vecPair[i].first, strValue.c_str());
				}
			}
		}
	} while (0);

	printf("\n");

	// HASH读写
	do
	{
		std::string strHashKey = "hash-user:1";
		std::string strName = "zhang3";
		uint32_t uAge = 30;

		CRedisClient* pRedis = redisFactory.getRedis( strHashKey );

		CResult result(true);
		result = pRedis->command("hmset %s name %s age %u", strHashKey.c_str(), strName.c_str(), uAge);
		if (!result)
		{
			printf("hset failed : %s \n", pRedis->getErrStr().c_str());
			break;
		}

		// 不同的redis命令会返回不同的结果类型，需调用相应的检测方法
		if (!result.isOK())
		{
			printf("hset failed : %s \n", pRedis->getErrStr().c_str());
			break;
		}

		printf("hset ok! \n");

		result = pRedis->command("hgetall %s", strHashKey.c_str());
		if (!result)
		{
			printf("hgetall failed : %s \n", pRedis->getErrStr().c_str());
			break;
		}

		if (!result.isArray())
		{
			printf("result not array \n");
			break;
		}

		// 采用HashResult这个帮助类方便取值
		CHashResult hashResult;

		for (size_t i = 0; i < result.getArraySize()-1; i+=2)
		{
			CResult subResultField(false), subResultValue(false);
			subResultField = result.getSubReply(i);
			subResultValue = result.getSubReply(i+1);

			std::string strField, strValue;
			subResultField.getString(strField);
			subResultValue.getString(strValue);

			printf("field:%s value:%s \n", strField.c_str(), strValue.c_str());

			hashResult.addKV(strField, strValue);
		}

		std::string strNameOut = hashResult.getValue("name", "");
		int nAgeOut = hashResult.getValue<int>("age", 0);
		printf("name:%s age:%d \n", strNameOut.c_str(), nAgeOut);

	} while (0);

	return 0;
}
