cmake_minimum_required(VERSION 2.8.1)

set (src main.cpp
    #./serverscenerunnermasterclient.cpp
    #./scenerunnermanager.pb.cc
    ../../dbaccess/runningscenedb.cpp

    ../../../mysqlcli/mysqlconnection.cpp
    ../../../mysqlcli/mysqlconnpool.cpp
    ../../../thread/mutexguard.cpp
    ../../../thread/mutexwrap.cpp
    ../../../thread/conditionwrap.cpp
    ../../../util/logwriter.cpp
    ../../../util/formatsql.cpp
    ../../../util/base64.cpp
    ../../../util/util.cpp
    ../../../config/configreader.cpp
)

set(libs 
    brpc
    gflags
    pthread
    protobuf
    mysqlclient
)

include_directories(../ ../../../brpc/output/include ../../ ../../../)
link_directories(../thread/ ../Crypt/ ../hiredis/ ../../../brpc/output/lib ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(testdb ${src})
target_link_libraries(testdb ${libs})


