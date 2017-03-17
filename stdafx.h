// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
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
