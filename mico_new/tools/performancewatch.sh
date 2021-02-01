#!/bin/bash
memory=`free -h |grep available|awk -F" "+ '{ print $4}' |awk -F"M" '{ print $1}'`
scenerunner_num=`ps -ef |grep slavenode_port |grep -v grep |wc -l`
conn_num=`ps -ef |grep conn_d |grep -v grep |wc -l`
Logical_num=`ps -ef |grep Logical |grep -v grep |wc -l`
fcgi_num=`ps -ef |grep fcgi |grep -v grep |wc -l`
while true 
do
	if [ $memory -lt 100 ];then
		echo "memory less than 100M in `date`" |heirloom-mailx -s "title" bingcang2357@163.com
	fi
	if [ $scenerunner_num -lt 3 ];then
		echo "scenerunner_num less than 3 in `date`" |heirloom-mailx -s "title" bingcang2357@163.com
	fi
	if [ $conn_num -eq 0 ];then
		echo "conn_num is 0 in `date`" |heirloom-mailx -s "title" bingcang2357@163.com
	fi
	if [ $Logical_num -eq 0 ];then
		echo "Logical_num is 0 in `date`" |heirloom-mailx -s "title" bingcang2357@163.com
	fi
	if [ $fcgi_num -eq 0 ];then
		echo "fcgi_num is 0 in `date`" |heirloom-mailx -s "title" bingcang2357@163.com
	fi
	sleep 10
done 
