cmake_minimum_required(VERSION 2.8.1)

# onlineinfo/deviceOfflineCommand.cpp
# onlineinfo/userofflineCommand.cpp
# onlineinfo/onlineinfo.cpp
set (src main.cpp
Business/Business.cpp
claimdevice.cpp
claimmanager.cpp
../util/util.cpp
../util/logwriter.cpp
../util/tcpdatapack.cpp 
../util/sock2string.cpp
../util/formatsql.cpp
../util/base64.cpp

../protocoldef/protocol.cpp
../protocoldef/protocolversion.cpp
../Message/Message.cpp
../Message/MsgReader.cpp
../Message/MsgWriter.cpp
../Message/Notification.cpp
../Message/relaymessage.cpp
../Message/tcpservermessage.cpp

../thread/mutexwrap.cpp
../thread/conditionwrap.cpp 
../thread/threadpool.cpp
../thread/mutexguard.cpp
../thread/threadwrap.cpp

../Crypt/tea.cpp
../Crypt/MD5.cpp

onlineinfo/mainthreadmsgqueue.cpp
onlineinfo/userdata.cpp
onlineinfo/devicedata.cpp
onlineinfo/mainthreadmsgqueue.cpp
onlineinfo/rediscache.cpp
onlineinfo/heartbeattimer.cpp
onlineinfo/shmcache.cpp
onlineinfo/devicetimeoutcommand.cpp
onlineinfo/usertimeoutcommand.cpp
onlineinfo/isonline.cpp
onlineinfo/onlineinfo.cpp
../msgqueue/shmwrap.cpp
../msgqueue/memoryhash.cpp
../config/configreader.cpp
../timer/timer.cpp
../timer/timerserver.cpp
../timer/clocktimer.cpp
../msgqueue/posixmsgqueue.cpp
../msgqueue/ipcmsgqueue.cpp
../msgqueue/procqueue.cpp
../mysqlcli/mysqlconnpool.cpp
../mysqlcli/mysqlconnection.cpp
./useroperator.cpp
./sessionkeygen.cpp
./deviceregister.cpp
../Message/itobytearray.cpp
./devicelogin.cpp 
./userpasswordrecover.cpp 
./mailsender.cpp 
./usermodifymail.cpp 
./messageencrypt.cpp 
./usermessageprocess.cpp 
./modifyuserhead.cpp
tcpserver/tcpserver.cpp 
./forwardmessage.cpp 
./messageforward.cpp 
./messagesender.cpp 
loginprocessor.cpp 
searchuser.cpp 
scenemessageprocess.cpp
messagereceiver.cpp
newmessagecommand.cpp
feedback.cpp
../serverinfo/serverinfo.cpp
./messagefromnotifysrv.cpp
./deletenotifymessage.cpp
../redisconnectionpool/redisconnectionpool.cpp
../redisconnectionpool/redispoolmanager.cpp
../redisconnectionpool/redisconns.cpp
./fileuploadredis.cpp
./gennotifyuseronlinestatus.cpp
./transferdevicecluster.cpp
./getserverlist.cpp
./getuserinfo.cpp
verfycodegen.cpp
./signalthread.cpp
./cacheduseroperator.cpp
../dbproxyclient/cacheddbuseroperator.cpp
../dbproxyclient/dbproxyclient.cpp
../dbproxyclient/dbproxyclientpool.cpp
../user/user.cpp
./deleteLocalCache.cpp
../serverinfo/serverid.cpp
./sceneConnectDevicesOperator.cpp
./sceneconnectordevices.cpp
./transferdevicetoanothercluster.cpp
./removemydevice.cpp
./deletedevicefromcluster.cpp

./dbaccess/user/userdao.cpp
./dbaccess/user/passworddao.cpp
./dbaccess/scene/scenedal.cpp
./dbaccess/scene/delscenefromfileserver.cpp
./dbaccess/scene/delsceneuploadlog.cpp
./dbaccess/cluster/clusterdal.cpp
./udpserver.cpp
./server.cpp
./loginatanotherclientnotify.cpp

./realmessagerepeatechecker.cpp
../udpsession/udpsession.cpp
../udpsession/messagerepeatchecker.cpp

test/testreadallonlinefromredis.cpp

./pushmessage/pushservice.cpp
./pushmessage/pushmessage.cpp
./pushmessage/newpushmessagenotify.cpp
./pushmessage/pushmessagesubscribethread.cpp

./runsceneatserverprocess.cpp
../servermonitormaster/dbaccess/runningscenedb.cpp
../servermonitormaster/serverscenerunnermasterclient.cpp
../servermonitormaster/scenerunnermanager.pb.cc
./rabbitmqservermonitor.cpp
./rabbitmqmessgeprocess.cpp
)


include_directories(../ ../brpc/output/include ../rabbitmq/)
link_directories(../thread/ ../Crypt/ ../hiredis/ ../brpc/output/lib ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wall -std=c++0x")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_CXX_FLAGS_DEBUG "-Wall -std=c++0x")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
#add_compile_options(-g)
#add_definitions(-D_DEBUG_VERSION)
#add_definitions(-DDEBUG_VERSION)

add_executable(LogicalSrv_d ${src})

target_link_libraries(LogicalSrv_d
libcrypt.a
libhiredis.a
pthread
rt
mysqlclient 
m
curl
event
event_core
event_pthreads
brpc
protobuf
amqpcpp
dl
)

