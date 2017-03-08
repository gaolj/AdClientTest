// AdClientTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#define BOOST_TEST_MODULE MainTest
#include <boost/test/unit_test.hpp>
namespace utf = boost::unit_test;

#include "stdafx.h"
#include "AdManager.h"
#include "AdManagerImpl.h"
#include "TcpClient.h"
#include "md5.hh"
#include <boost/algorithm/string.hpp>	// boost::to_upper_copy

using namespace boost::asio::ip;
using boost::asio::io_service;
using std::shared_ptr;
using boost::thread;

static int count;
static src::severity_channel_logger<SeverityLevel> logger((keywords::channel = "net"));
static auto locolhost = tcp::endpoint(address::from_string("127.0.0.1"), 18888);

BOOST_AUTO_TEST_CASE(tc_init_logger)
{
	initLogger();
}

BOOST_AUTO_TEST_CASE(singleClientTest)
{
	LOG_INFO(logger) << "BGN singleClientTest";

	AdManager& adManager = AdManager::getInstance();
	adManager.setConfig("127.0.0.1", 18888, 123456, false, 0);
	adManager.bgnBusiness();

	LOG_INFO(logger) << "END singleClientTest";
}

// ����
BOOST_AUTO_TEST_CASE(multiClientTest)
{
	std::cout << "����ͻ�����: " << std::endl;
	std::cin >> count;

	LOG_INFO(logger) << "BGN multiClientTest";

	io_service ios;
	shared_ptr<io_service::work> work(new io_service::work(ios));
	auto t1 = thread([&ios]() {ios.run(); });

	std::set<shared_ptr<TcpClient>> tcpClientPool;
	for (int i = 0; i < count; i++)
	{
		auto tcpClient = std::make_shared<TcpClient>(ios);
		tcpClient->setAutoReconnect(false);
		tcpClient->asyncConnect(locolhost);
		tcpClientPool.insert(tcpClient);
	}

	int sum = 0;
	LOG_INFO(logger) << "�ȴ����пͻ��������Ϸ���ˣ��ȴ�3��";
	boost::this_thread::sleep(boost::posix_time::seconds(3));
	for (auto client : tcpClientPool)
		if (client->isConnected())
			sum++;
	LOG_INFO(logger) << "ͬʱ������" << sum << "������";

	// getAdPlayPolicy
	int id = htonl(123456);
	char bufId[4] = {};
	memcpy(bufId, &id, 4);
	Message msgReq;
	msgReq.set_content(bufId, 4);
	msgReq.set_method("getAdPlayPolicy");
	AdPlayPolicy policy;
	std::vector<boost::future<Message>> setFutures;
	for (auto client : tcpClientPool)
		setFutures.push_back(client->session()->request(msgReq));

	sum = 0;
	boost::chrono::seconds span(3);
	for (auto& fut : setFutures)
		if (fut.wait_for(span) == boost::future_status::ready)
		{
			sum++;
			Message msgRsp = fut.get();
			if (msgRsp.returncode() != 0 || policy.ParseFromString(msgRsp.content()) == false)
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdPlayPolicyʧ��:" << msgRsp.returnmsg() << msgRsp.returncode();
			}
		}
	LOG_INFO(logger) << sum << "���ͻ��˳ɹ��յ�AdPlayPolicy";

	// getAdList
	Result ads;
	sum = 0;
	setFutures.clear();
	msgReq.set_method("getAdList");
	for (auto client : tcpClientPool)
		setFutures.push_back(client->session()->request(msgReq));
	for (auto& fut : setFutures)
		if (fut.wait_for(span) == boost::future_status::ready)
		{
			sum++;
			Message msgRsp = fut.get();
			if (msgRsp.returncode() != 0 || ads.ParseFromString(msgRsp.content()) == false) // ???policy.ParseFromStringҲ��
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdListʧ��:" << msgRsp.returnmsg() << msgRsp.returncode();
			}
		}
	std::unordered_map<uint32_t, Ad> _mapAd;
	for (int i = 0; i < ads.ads_size(); i++)
	{
		auto ad = *ads.mutable_ads(i);
		utf8ToGB2312(ad);
		_mapAd.insert(std::make_pair(ad.id(), ad));
	}
	LOG_INFO(logger) << sum << "���ͻ��˳ɹ��յ�AdList";

	// getAdFile
	sum = 0;
	setFutures.clear();
	int adID = 0;
	for (auto pair : _mapAd)
		if (pair.second.download_size() != 0)	// ��Ҫ����
		{
			adID = pair.first;
			break;
		}

	msgReq.set_content(&adID, 4);
	msgReq.set_method("getAdFile");
	for (auto client : tcpClientPool)
		setFutures.push_back(client->session()->request(msgReq));
	for (auto& fut : setFutures)
		if (fut.wait_for(span) == boost::future_status::ready)
		{
			Message msgRsp = fut.get();
			if (msgRsp.returncode() != 0)
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdFileʧ��:" << msgRsp.returnmsg() << msgRsp.returncode();
			}
			else
			{
				if (_mapAd[adID].size() == msgRsp.content().length())
					sum++;
			}
		}
	LOG_INFO(logger) << sum << "���ͻ��˳ɹ��յ�" << _mapAd[adID].filename() << "�ļ���С��" << _mapAd[adID].size() << " bytes";

	tcpClientPool.clear();
	work.reset();
	t1.join();
	LOG_INFO(logger) << "END multiClientTest" << "\n";
}

BOOST_AUTO_TEST_CASE(tc_end_AdServer)
{
	LOG_INFO(logger) << "����������س���������һ�����";
	char c;
	std::cin >> c;
}

