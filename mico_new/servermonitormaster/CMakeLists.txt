cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)

set (src ./server.cpp
    ./scenerunnermanager.pb.cc
    ./serverrpc.cpp
    ./dbaccess/runningscenedb.cpp
    ./dbaccess/runningscenedb.cpp
    runnernodemanager.cpp
    scenerepo.cpp
    ./runnerregister.pb.cc
    ./runnerregisterserviceimp.cpp
    ./runneraddscene.pb.cc
    ./runneraddsceneclient.cpp

    ../mysqlcli/mysqlconnection.cpp
    ../mysqlcli/mysqlconnpool.cpp
    ../thread/mutexguard.cpp
    ../thread/mutexwrap.cpp
    ../thread/conditionwrap.cpp
    ../util/logwriter.cpp
    ../util/formatsql.cpp
    ../util/base64.cpp
    ../config/configreader.cpp
    ../util/util.cpp
)

set (src1 mainslave.cpp
    ./serverscenerunnermasterclient.cpp
    ./scenerunnermanager.pb.cc
    ./dbaccess/runningscenedb.cpp
    ./registernodedetail.cpp
    ./runnerregister.pb.cc
    ./runnerregisterclient.cpp
    ./runneraddscene.pb.cc
    ./runneraddsceneimp.cpp
    ./runningscenemanager.cpp
    ./scenerunner.cpp
    ./hmiproject/alarm.cpp
    ./hmiproject/connection.cpp
    ./hmiproject/hmiproject.cpp
    ./hmiproject/dialog.cpp
    ./hmiproject/projectrunner.cpp
    ./hmiproject/variable.cpp
    ./hmiproject/util2/datadecode.cpp
    ./rabbitmqservice.cpp

    ../mysqlcli/mysqlconnection.cpp
    ../mysqlcli/mysqlconnpool.cpp
    ../thread/mutexguard.cpp
    ../thread/mutexwrap.cpp
    ../thread/conditionwrap.cpp
    ../util/logwriter.cpp
    ../util/formatsql.cpp
    ../util/base64.cpp
    ../util/util.cpp
    ../util/sock2string.cpp
    ../config/configreader.cpp
    ../RelaySrv/sessionRandomNumber.cpp

    ./serverudp/serverudp.cpp
    ./serverudp/isock.cpp
    #./hmicom/commInterface.cpp
    #./hmicom/commService.cpp
    ./hmicom/commInterface.cpp
    ./hmicom/commservice.cpp
    ./hmicom/dvpService.cpp
    ./hmicom/fatekService.cpp
    ./hmicom/gateWayService.cpp
    ./hmicom/miprotocol4Service.cpp
    ./hmicom/modbusService.cpp
    ./hmicom/ppimpiService.cpp
    ./hmicom/rwsocket.cpp
    ./hmicom/gateWay/udpL1L2.cpp
    ./hmicom/gateWay/udpsocket.cpp
    ./hmicom/cnetService.cpp
    ./hmicom/finHostlinkService.cpp
    ./hmicom/FPService.cpp
    ./hmicom/hostlinkService.cpp
    ./hmicom/mewtocolService.cpp
    ./hmicom/miprofxService.cpp
    ./hmicom/mpi300Service.cpp
    ./hmicom/tcpModbus/modbusmsg.cpp
    ./hmicom/tcpModbus/tcpmodbus.cpp
    ./hmicom/tcpModbusService.cpp

    ../LogicalSrv/onlineinfo/devicedata.cpp
    ../LogicalSrv/onlineinfo/userdata.cpp
    ../LogicalSrv/onlineinfo/rediscache.cpp
    ../redisconnectionpool/redisconnectionpool.cpp
    ../redisconnectionpool/redisconns.cpp
    ../redisconnectionpool/redispoolmanager.cpp
    ../protocoldef/protocol.cpp
    ../Message/Message.cpp
    ../Crypt/MD5.cpp
    ../Crypt/tea.cpp
)

set(libs 
    brpc
    gflags
    pthread
    protobuf
    mysqlclient
    event_core
    amqpcpp
    dl
   )

include_directories(. ../ ../brpc/output/include)
link_directories(../thread/ ../Crypt/ ../hiredis/ ../brpc/output/lib ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
add_definitions(-DHMI_ANDROID_IOS)

add_executable(mointormaster ${src})
add_executable(scenerunner ${src1})

target_link_libraries(mointormaster ${libs})
target_link_libraries(scenerunner ${libs} Qt5::Core Qt5::Widgets Qt5::Network hiredis)


