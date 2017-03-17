// AdClientTest.cpp : 定义控制台应用程序的入口点。
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
#include <boost/locale.hpp>				// boost::locale::conv::from_utf
#include <boost/thread/tss.hpp> 
#include <sstream>
#include <deque>

using boost::asio::io_service;
using boost::thread;
using std::shared_ptr;

extern tcp::endpoint serverAddr;
int cntClient, cntSend, cntThread;
static src::severity_channel_logger<SeverityLevel> logger((keywords::channel = "net"));

boost::chrono::seconds span(1 * 60);
boost::thread_specific_ptr<AdPlayPolicy> _policy;
boost::thread_specific_ptr<Result> _adList;

void getAdPlayPolicy(std::vector<shared_ptr<TcpClient>> clients);
void getAdList(std::vector<shared_ptr<TcpClient>> clients);
void getAdFile(std::vector<shared_ptr<TcpClient>> clients);
void getAll(std::vector<shared_ptr<TcpClient>> clients);
void test();

BOOST_AUTO_TEST_CASE(oneClientOneThread, *utf::enable_if<enable_oneClientOneThread>())
{
	std::cout << "单客户机，单线程，输入请求次数: ";
	std::cin >> cntSend;
	cntClient = 1;
	cntThread = 1;
	LOG_INFO(logger) << "客户机-1, 线程-1, 请求次数-" << cntSend;

	LOG_INFO(logger) << "BGN 单机单线程";
	test();
	LOG_INFO(logger) << "END 单机单线程" << "\n";
}

BOOST_AUTO_TEST_CASE(oneClientMultiThread, *utf::enable_if<enable_oneClientMultiThread>())
{
	std::cout << "单客户机，多线程，输入线程数: ";
	std::cin >> cntThread;
	std::cout << "单客户机，多线程，输入请求次数: ";
	std::cin >> cntSend;
	cntClient = 1;
	LOG_INFO(logger) << "客户机-1, 线程-" << cntThread << ", 请求次数-" << cntSend;

	LOG_INFO(logger) << "BGN 单机多线程";
	test();
	LOG_INFO(logger) << "END 单机多线程" << "\n";
}

BOOST_AUTO_TEST_CASE(multiClientOneThread, *utf::enable_if<enable_multiClientOneThread>())
{
	std::cout << "多客户机，单线程, 输入客户机数: ";
	std::cin >> cntClient;
	std::cout << "多客户机，单线程, 输入请求次数: ";
	std::cin >> cntSend;
	cntThread = 1;
	LOG_INFO(logger) << "客户机-" << cntClient << ", 线程-1, 请求次数-" << cntSend;

	LOG_INFO(logger) << "BGN 多机单线程";
	test();
	LOG_INFO(logger) << "END 多机单线程" << "\n";
}

BOOST_AUTO_TEST_CASE(multiClientMultiThread, *utf::enable_if<enable_multiClientMultiThread>())
{
	std::cout << "多线程，多客户机，输入客户机数: ";
	std::cin >> cntClient;
	std::cout << "多线程，多客户机，输入线程数: ";
	std::cin >> cntThread;
	std::cout << "多线程，多客户机，输入请求次数: ";
	std::cin >> cntSend;
	LOG_INFO(logger) << "客户机-" << cntClient << ", 线程-" << cntThread << ", 请求次数-" << cntSend;

	LOG_INFO(logger) << "BGN 多机多线程";
	test();
	LOG_INFO(logger) << "END 多机多线程" << "\n";
}

BOOST_AUTO_TEST_CASE(tcNClientNThread)
{
	LOG_INFO(logger) << "按任意键及回车，进入下一组测试";
	char c;
	std::cin >> c;
}

void getAdList(std::vector<shared_ptr<TcpClient>> clients)
{
	if (_adList.get() == 0)
		_adList.reset(new Result());

	// getAdList
	Message msgReq;
	msgReq.set_method("getAdList");
	std::deque<boost::future<Message>> futures;
	for (int i = 0; i < cntClient; i++)
		for (int j = 0; j < cntSend; j++)
			futures.push_back(clients[i]->session()->request(msgReq));

	int sum = 0;
	while (!futures.empty())
		if (futures.front().wait_for(span) == boost::future_status::ready)
		{
			Message msgRsp = futures.front().get();
			if (msgRsp.returncode() == 0 && _adList->ParseFromString(msgRsp.content()) == true) // ???policy.ParseFromString也对
				sum++;
			else
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdList失败:" << msgRsp.returnmsg() << msgRsp.returncode();
			}
			futures.pop_front();
		}
	LOG_INFO(logger) << sum << "次收到AdList";
}

void getAdPlayPolicy(std::vector<shared_ptr<TcpClient>> clients)
{
	if (_policy.get() == 0)
		_policy.reset(new AdPlayPolicy());

	int id = htonl(123456);
	char bufId[4] = {};
	memcpy(bufId, &id, 4);

	Message msgReq;
	msgReq.set_method("getAdPlayPolicy");
	msgReq.set_content(bufId, 4);
	std::deque<boost::future<Message>> futures;
	for (int i = 0; i < cntSend; i++)
		for (int j = 0; j < cntClient; j++)
			futures.push_back(clients[j]->session()->request(msgReq));

	int sum = 0;
	while (!futures.empty())
		if (futures.front().wait_for(span) == boost::future_status::ready)
		{
			Message msgRsp = futures.front().get();
			if (msgRsp.returncode() == 0 && _policy->ParseFromString(msgRsp.content()) == true)
				sum++;
			else
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdPlayPolicy失败:" << msgRsp.returnmsg() << msgRsp.returncode();
			}
			futures.pop_front();
		}
	LOG_INFO(logger) << sum << "次收到AdPlayPolicy";
}

void getAdFile(std::vector<shared_ptr<TcpClient>> clients)
{
	int adID = 0;
	int adSize = 0;
	std::string filename;
	for (auto ad : _adList->ads())
		if (ad.download_size() != 0)	// 需要下载
		{
			adID = ad.id();
			adSize = ad.size();
			filename = ad.filename();
			break;
		}

	Message msgReq;
	msgReq.set_method("getAdFile");
	msgReq.set_content(&adID, 4);
	std::deque<boost::future<Message>> futures;
	for (int i = 0; i < cntSend; i++)
		for (int j = 0; j < cntClient; j++)
			futures.push_back(clients[j]->session()->request(msgReq));

	int sum = 0;
	while (!futures.empty())
		if (futures.front().wait_for(span) == boost::future_status::ready)
		{
			Message&& msgRsp = futures.front().get();
			if (msgRsp.returncode() == 0)
			{
				if (adSize == msgRsp.content().length())
					sum++;
			}
			else
			{
				utf8ToGB2312(msgRsp);
				LOG_ERROR(logger) << "getAdFile失败:" << msgRsp.returnmsg() << ", errorcode:" << msgRsp.returncode() << ", msgID:" << msgRsp.id();
			}
			futures.pop_front();
		}
	LOG_INFO(logger) << sum << "次收到(" << boost::locale::conv::from_utf(filename, "gb2312") << "), 文件大小:" << adSize << " bytes";
}

void getAll(std::vector<shared_ptr<TcpClient>> clients)
{
	getAdPlayPolicy(clients);
	getAdList(clients);
	getAdFile(clients);
}

void test()
{
	io_service ios;
	shared_ptr<io_service::work> work(new io_service::work(ios));
	auto t0 = thread([&ios]() { ios.run(); });

	std::vector<shared_ptr<TcpClient>> clients;
	for (int i = 0; i < cntClient; i++)
	{
		auto client = std::make_shared<TcpClient>(ios);
		client->setAutoReconnect(false);
		client->asyncConnect(serverAddr);
		client->session()->_requestHandler = [](Message msg) { LOG_ERROR(logger) << "收到错误回应：" << msg.id(); };
		clients.push_back(client);
	}

	// 在cntThread个线程中同时发出请求
	std::vector<thread> threads;
	for (int i = 0; i < cntThread; i++)
		threads.push_back(thread(getAll, clients));

	for (int i = 0; i < cntThread; i++)
		threads[i].join();
	for (int i = 0; i < cntClient; i++)
		clients[i]->stop();

	work.reset();
	t0.join();
}
