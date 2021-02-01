cmake_minimum_required(VERSION 2.8.1)

set (src main.cpp
./connectionserver.cpp
./tcpconnection.cpp
./tcpclientmessage.cpp
../Message/tcpservermessage.cpp
./connidgen.cpp
./logicalservers.cpp
./serverslave.cpp
./serverqueue.cpp
./connectionservermessage.cpp
./udpserver.cpp
./utilsock.cpp

../util/util.cpp
../util/logwriter.cpp
../util/sock2string.cpp

../Crypt/tea.cpp
../Crypt/MD5.cpp

../Message/Message.cpp

../thread/mutexwrap.cpp
../thread/mutexguard.cpp
../thread/threadwrap.cpp

../timer/wheeltimer.cpp

../config/configreader.cpp
../serverinfo/serverinfo.cpp
../serverinfo/serverid.cpp
../mysqlcli/mysqlconnection.cpp
)


include_directories(../)
link_directories(../thread/ ../Crypt/ ../hiredis/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(conn_d ${src})

target_link_libraries(conn_d 
#libcrypt.a
#libhiredis.a
pthread
rt
mysqlclient 
m
#curl
event_core
event_pthreads
)

