# libtinyredis
一个轻量级的redis客户端库实现

编译说明：

首先安装hiredis:
官方网址：
http://redis.cn/
下载，解压和编译：
$ wget http://redis.googlecode.com/files/redis-2.6.7.tar.gz
$ tar xzf redis-2.6.7.tar.gz
$ cd redis-2.6.7
$ make
通常情况下，会在deps/hiredis下生成libhiredis.a库。
接下来，可执行参下命令，手动安装：
mkdir /usr/local/include/hiredis 
cd deps/hiredis
cp *.h /usr/local/include/hiredis/
cp libhiredis.a /usr/local/lib/

其次编译libtinyredis:
make all
在tinyredis目录下会生成4个lib文件，分别如下：
1）libtinyredis.a  单线程debug版本
2）libtinyredis_mt.a 多线程debug版本
3）libtinyredis.ra 单线程release版本
4）libtinyredis_mt.ra 多线程release版本
另外，sample.cpp是使用样例，介绍了如何对单KV、多单V、hash结构进行读写访问。


使用说明：

头文件：
#include "RedisFactory.h"

方法介绍：

1）添加分布式实例：
    void CRedisFactory::addRedis(const std::string& strIp, uint16_t uPort16, const std::string& strPass, uint32_t uMiniSeconds)
2）根据key获取实例对象：
    CRedisClient* CRedisFactory::getRedis(uint32_t uKey);
    CRedisClient* CRedisFactory::getRedis(const std::string& strKey);
3）执行命令：
    redisReply* CRedisClient::command(const char* szFormat, ...);
4）访问结果集：
    CResult::CResult(bool bAutoFree = true);  构造结果集，参数表示是否自动释放资源
    bool CResult::isArray();  结果集是否数组
    bool CResult::isInteger();  结果集是否整形
    bool CResult::isString(); 结果集是否字符串
    bool CResult::isNil(); 结果集是否未命中
    bool CResult::isStatus(); 结果集是否操作成功
    redisReply* CResult::getSubReply(size_t uPos); 获取数组的子结果
    size_t CResult::getArraySize(); 获取数组元素大小
    int64_t CResult::getInteger(); 获取整形值
    void CResult::getString(std::string& str); 获取字符串
    bool CResult::isOK(); 检查操作是否成功

以下是从sample.cpp提取的单KV读写访问示例代码：

    using namespace tinyredis;

    // 创建工厂实例
    CRedisFactory redisFactory;

    // 初使化redis服务配置
    redisFactory.addRedis("127.0.0.1", 3000, "123456", 1000);
    redisFactory.addRedis("127.0.0.1", 3001, "123456", 1000);

    uint32_t uId = 1;
    std::string strName = "zhang3";

    CRedisClient* pRedis = redisFactory.getRedis(uId);
    CResult result(true);
    result = pRedis->command("set name:%u %s", uId, strName.c_str());
    if (!result)
    {
        printf("set failed : %s \n", pRedis->getErrStr().c_str());
        return;
    }

    printf("set ok \n");

    result = pRedis->command("get name:%u", uId);
    if (!result)
    {
        printf("get failed : %s \n", pRedis->getErrStr().c_str());
        return;
    }

    if (result.isNil())
    {
        printf("not found! \n");
        return;
    }

    if (result.isString())
    {
        std::string strValue;
        result.getString(strValue);
        printf("get value : %s \n", strValue.c_str());
    }
