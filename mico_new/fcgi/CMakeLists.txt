cmake_minimum_required(VERSION 3.1.0)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
	set(CMAKE_INCLUDE_CURRENT_DIR ON) 
endif()

set(src fcgiMain.cpp 
	fcgiEnv.cpp 
	WXBizDataCrypt.cpp 
	WXBizMsgCrypt.cpp 
	urlDecode.cpp 
	../util/logwriter.cpp 
	../mysqlcli/mysqlconnection.cpp 
	../mysqlcli/mysqlconnpool.cpp 
	../util/formatsql.cpp  
	../config/configreader.cpp 
	../util/util.cpp
    redisConnPool/redisConnPool.cpp 
	../redisconnectionpool/redisconnectionpool.cpp
	)

set(libs 
	pthread
	protobuf
	mysqlclient
	amqpcpp
	dl
	threadpool
	hiredis
	fcgi
	crypto
	tinyxml2
	ssl
	event
	)
include_directories(. ../ ../../)
link_directories(../hiredis )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

add_executable(fcgi-bin ${src})
target_link_libraries(fcgi-bin ${libs})

