cmake_minimum_required(VERSION 2.8.1)

set (src
main.cpp
../../dbproxyclient/dbproxyclient.cpp
../../dbproxyclient/cacheddbuseroperator.cpp
../../dbproxyclient/dbproxyclientpool.cpp
../../util/logwriter.cpp
../../user/user.cpp
../../util/util.cpp
../../util/tcpdatapack.cpp
)

include_directories(../../)
link_directories(../../thread/ ../../Crypt/ ../../hiredis/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -O2")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(client ${src})

target_link_libraries(client
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

