cmake_minimum_required(VERSION 2.8.1)

set (src
main.cpp
../util/util.cpp
../util/logwriter.cpp
../config/configreader.cpp
../thread/mutexwrap.cpp
../thread/mutexguard.cpp
../thread/conditionwrap.cpp
../redisconnectionpool/redisconns.cpp
../redisconnectionpool/redisconnectionpool.cpp
../redisconnectionpool/redispoolmanager.cpp
../mysqlcli/mysqlconnection.cpp
)

include_directories(../)
link_directories(../thread/ ../Crypt/ ../hiredis/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(changefirm ${src})

target_link_libraries(changefirm 
libcrypt.a
libhiredis.a
pthread
rt
mysqlclient 
m
curl
)

