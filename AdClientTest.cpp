// AdClientTest.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "AdManager.h"
#include <iostream>

int _tmain(int argc, _TCHAR* argv[])
{
	AdManager& adManager = AdManager::getInstance();
	adManager.setConfig("127.0.0.1", 18888, 123456, false, 0);
	adManager.bgnBusiness();
	
	int i;
	std::cin >> i;
	
	return 0;
}

