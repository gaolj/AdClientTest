#include <boost/test/unit_test.hpp> 
namespace utf = boost::unit_test;

#include "stdafx.h"
#include "Logger.h"
#include "TcpSession.h"
#include "TcpClient.h"
#include "TcpServer.h"

using namespace boost::asio::ip;
using boost::asio::io_service;
using std::shared_ptr;
using boost::thread;

int count = 1;
static src::severity_channel_logger<SeverityLevel> logger((keywords::channel = "net"));
extern tcp::endpoint serverAddr;

// 模式：连接-（断开-自动重连）-（断开-自动重连）......
// 目的：测试TcpClient断开后自动重连功能
BOOST_AUTO_TEST_CASE(auto_reconnect, *utf::enable_if<enable_connect>())
{
	std::cout << std::endl << "测试TcpClient断开后自动重连功能" << std::endl;
	std::cout << "输入自动重连次数: ";
	std::cin >> count;

	LOG_INFO(logger) << "BGN auto_reconnect";

	io_service ios;
	shared_ptr<io_service::work> work(new io_service::work(ios));
	auto t1 = thread([&ios]() {ios.run(); });

	TcpClient tcpClient(ios);
	if (!tcpClient.syncConnect(serverAddr))
	{
		LOG_INFO(logger) << "连接中心失败";
		return;
	}

	for (int i = 0; i < count; i++)
	{
		tcpClient.stop();
		boost::this_thread::sleep(boost::posix_time::milliseconds(100));
	}

	tcpClient.setAutoReconnect(false);
	tcpClient.stop();
	work.reset();
	t1.join();
	LOG_INFO(logger) << "END auto_reconnect" << "\n";
}

// 模式：连接-断开-连接-断开.....
// 目的：测试TcpClient是否存在内存泄漏、socket句柄泄漏问题
BOOST_AUTO_TEST_CASE(sync_connect, *utf::enable_if<enable_connect>())
{
	std::cout << std::endl << "测试同步连接功能, 连接-断开-连接-断开....." << std::endl;
	std::cout << "输入同步连接次数: ";
	std::cin >> count;

	LOG_INFO(logger) << "BGN sync_connect";

	io_service ios;
	shared_ptr<io_service::work> work(new io_service::work(ios));
	auto t1 = thread([&ios]() {ios.run(); });

	int ok = 0;
	for (int i = 0; i < count; i++)
	{
		auto tcpClient = std::make_shared<TcpClient>(ios);
		tcpClient->setAutoReconnect(false);
		if (tcpClient->syncConnect(serverAddr))
			ok++;
	}
	LOG_INFO(logger) << "成功连接" << ok << "次";

	work.reset();
	t1.join();
	LOG_INFO(logger) << "END sync_connect" << "\n";
}

// 模式：连接-连接..., 断开-断开...
// 目的：测试TcpClient是否存在内存泄漏、socket句柄泄漏问题，及中心的大并发连接支持
BOOST_AUTO_TEST_CASE(multi_connect, *utf::enable_if<enable_connect>())
{
	std::cout << std::endl << "测试异步连接功能, 连接-连接..., 断开-断开..." << std::endl;
	std::cout << "输入并发连接数: ";
	std::cin >> count;

	LOG_INFO(logger) << "BGN multi_connect";

	io_service ios;
	shared_ptr<io_service::work> work(new io_service::work(ios));
	auto t1 = thread([&ios]() {ios.run(); });
	std::set<shared_ptr<TcpClient>> tcpClientPool;

	for (int i = 0; i < count; i++)
	{
		auto tcpClient = std::make_shared<TcpClient>(ios);
		tcpClient->setAutoReconnect(false);
		tcpClient->asyncConnect(serverAddr);
		tcpClientPool.insert(tcpClient);
	}

	int count = 0;
	boost::this_thread::sleep(boost::posix_time::seconds(3));
	for (auto client : tcpClientPool)
		if (client->isConnected())
			count++;
	LOG_INFO(logger) << "同时建立了" << count << "个连接";

	tcpClientPool.clear();
	work.reset();
	t1.join();
	LOG_INFO(logger) << "END multi_connect" << "\n";
}

BOOST_AUTO_TEST_CASE(tc_end_connect, *utf::enable_if<enable_connect>())
{
	LOG_INFO(logger) << "按任意键及回车，进入下一组测试";
	char c;
	std::cin >> c;
}
