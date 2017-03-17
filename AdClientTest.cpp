#include <boost/test/unit_test.hpp> 
namespace utf = boost::unit_test;

#include "stdafx.h"
#include "Logger.h"
#include <iostream>

tcp::endpoint serverAddr;
BOOST_AUTO_TEST_CASE(tcInitLogger)
{
	std::string ip;
	int port;
	std::cout << "输入网吧服务器IP: ";
	std::cin >> ip;
	std::cout << "输入网吧服务器端口: ";
	std::cin >> port;
	if (ip == "1")
		ip = "127.0.0.1";
	if (port == 1)
		port = 18888;
	serverAddr = tcp::endpoint(address::from_string(ip), port);

	initLogger(debug);
}

