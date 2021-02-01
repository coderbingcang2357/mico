cmake_minimum_required(VERSION 2.8.1)

set (src
main.cpp
../util/util.cpp
../util/sock2string.cpp
../util/logwriter.cpp
RelayReqHandler.cpp
ChannelManager.cpp
./relaythread.cpp
./relayports.cpp
./mainqueue.cpp
./relayinternalmessage.cpp
../msgqueue/ipcmsgqueue.cpp
../msgqueue/posixmsgqueue.cpp
../Message/Message.cpp
../Message/relaymessage.cpp
../msgqueue/ipcmsgqueue.cpp
../msgqueue/posixmsgqueue.cpp
../Message/MsgReader.cpp
../Message/MsgWriter.cpp
../config/configreader.cpp
./messagethread.cpp
./sessionRandomNumber.cpp
../protocoldef/protocol.cpp

../thread/mutexwrap.cpp
../thread/mutexwrap.cpp
../thread/threadpool.cpp
../thread/threadwrap.cpp
../thread/mutexguard.cpp
../thread/conditionwrap.cpp

../Crypt/tea.cpp
../Crypt/MD5.cpp

../mysqlcli/mysqlconnpool.cpp
../mysqlcli/mysqlconnection.cpp

./channelDatabase.cpp
)

include_directories(../)
link_directories(../thread/ ../Crypt/ ../hiredis/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(RelaySrv_d ${src})

target_link_libraries(RelaySrv_d 
#threadpool
#libcrypt.a
libhiredis.a
pthread
rt
mysqlclient 
m
curl
event_core
event_pthreads
)

