// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <boost/asio/ip/tcp.hpp>		// tcp::socket, tcp::acceptor
using namespace boost::asio::ip;


const bool enable_connect = true;
const bool enable_oneClientOneThread = true;
const bool enable_oneClientMultiThread = true;
const bool enable_multiClientOneThread = true;
const bool enable_multiClientMultiThread = true;
