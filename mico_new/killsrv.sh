#!/bin/sh

killall -s USR1 LogicalSrv_d
killall NotifySrv_d
killall RelaySrv_d
killall DelSceneSrv_d #此进程在文件服务器上运行
killall conn_d
#killall Logserver
killall micoreport_d
killall mointormaster 
killall scenerunner 
#killall msgqrm
# 微信开发者服务器
fcgiexist=`ps -ef |grep fcgi-bin |grep -v grep`
if [ -n "$fcgiexist" ];then
	ps -ef |grep fcgi-bin |grep -v grep |awk -F" "+ '{ print $2}' |sudo xargs kill -9
fi
