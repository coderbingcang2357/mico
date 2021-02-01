ps -ef |grep fcgi-bin |grep -v grep |awk -F" "+ '{print $2}' |sudo xargs kill -9
sudo g++ -g fcgiMain.cpp fcgiEnv.cpp WXBizDataCrypt.cpp WXBizMsgCrypt.cpp urlDecode.cpp ../util/logwriter.cpp ../mysqlcli/mysqlconnection.cpp ../mysqlcli/mysqlconnpool.cpp ../util/formatsql.cpp  ../config/configreader.cpp ../util/util.cpp\
	redisConnPool/redisConnPool.cpp ../redisconnectionpool/redisconnectionpool.cpp\
	 -I .. -o fcgi-bin  -L /usr/local/lib -L ../hiredis -lthreadpool -lmysqlclient -lhiredis\
	-lfcgi -lcrypto -ltinyxml2 -lssl -lpthread -levent -lamqpcpp -ldl
sudo spawn-fcgi fcgi-bin -a 0.0.0.0 -p 8089 -f fcgi_error.log

