针对claymore抽水的壳程序

当前版本支持（已经测试）claymore的ETC_ETH_ETP9.8版本
支持的挖矿方式为：ETC单挖、ETH单挖、ETP单挖、ETC+SC双挖、ETH+SC双挖、ETP+SC双挖，使用时修改dist/对应的bat文件，示例：以太币挖矿ETH.bat
withdll.exe -epool eth.f2pool.com:8008 -ewal 0x88aB6f58FFD714608734A69663bF5A3083e099DF -epsw x -eworker eth1.0 -mode 1 -allpools 1 -allcoins 1

源编译：
1、依赖环境：vs2017
2、vi Source\claymore\win32sock\compile.bat 修改对应的环境路径
3、nmake