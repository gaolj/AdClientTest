﻿此测试程序，主要测试静态库AdManager.lib在网吧客户端的功能，及广告服务端模块的稳定性、CPU和内存的占用

一、模似多客户机的并发请求，用例如下：
	单客户机，单线程，多次请求
	单客户机，多线程，多次请求
	多客户机，单线程，多次请求
	多客户机，多线程，多次请求


二、测试各种TCP连接状况，用例如下：
	测试广告客户端在TCP断开后的自动重连功能
	测试单客户机一次一次连接，同步连接功能, 连接-断开-连接-断开.....
	测试多客户机同时发起连接，异步连接功能, 连接-连接..., 断开-断开...


使用方式：
	前提条件：AdServerTest.exe已经运行。
	直接双击运行AdClientTest.exe，在控制台窗口里，根据提示输入：服务器(运行AdServerTest.exe的机器)地址、端口，测试数量等

	查看控制台日志输出
	查看文件日志输出:log/advert_000.log
	查看Windows任务管理器查看CPU使用、内存占用、句柄数