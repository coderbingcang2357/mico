cmake_minimum_required(VERSION 2.8.1)

set (src
main.cpp
server.cpp
users.cpp
loadFromdb.cpp
../util/util.cpp
../util/tcpdatapack.cpp
../user/user.cpp
./connection.cpp
./commandprocessor.cpp
../config/configreader.cpp
../mysqlcli/mysqlconnpool.cpp
../mysqlcli/mysqlconnection.cpp
../util/logwriter.cpp
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

add_executable(dbproxy_d ${src})

target_link_libraries(dbproxy_d
threadpool
libhiredis.a
pthread
rt
mysqlclient 
m
curl
event_core
event_pthreads
)

