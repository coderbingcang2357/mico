#!/bin/sh

runserverprocess ()
{
    local cnt=$(ps -e | grep "$1" | wc -l)
	if [ "$1" = "scenerunner" -a $cnt -lt 3 ]; then
        ./$@ &
	elif [ $cnt -eq 0 ] ; then
        ./$@ &
    fi
}

runWXDevelopServer()
{
	sudo spawn-fcgi fcgi-bin -a 0.0.0.0 -p 8089 & 
}


runserverprocess conn_d  "-daemon" "-threadconn4" 
#runserverprocess Logserver "-daemon" 
runserverprocess micoreport_d "-daemon" 
runserverprocess LogicalSrv_d  "-daemon" "-thread20" 
runserverprocess NotifySrv_d "-daemon" 
runserverprocess RelaySrv_d "-daemon" 
runserverprocess DelSceneSrv_d /data/upfiles/files "-daemon"
runserverprocess mointormaster "-daemon" 

#从节点要等主节点先启动好，再启动
mointormaster_start=`ps -ef |grep mointormaster |grep -v grep`
if [ -n "$mointormaster_start" ]; then
	runserverprocess scenerunner 8001 "-daemon" 
	runserverprocess scenerunner 8002 "-daemon" 
	runserverprocess scenerunner 8003 "-daemon" 
fi
#runserverprocess msgqrm $@

#微信公众号和小程序开发服务器
runWXDevelopServer

ps -e | grep -E "_d$|mointormaster|scenerunner|fcgi-bin"
pcount=$(ps -e | grep -E "_d$|mointormaster|scenerunner|fcgi-bin" | wc -l)
echo "Find process count: ${pcount}"
