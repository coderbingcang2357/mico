cmake_minimum_required(VERSION 2.8.1)

set(src main.cpp
BusinessManager.cpp
../util/util.cpp
../util/logwriter.cpp
../util/sock2string.cpp
../mysqlcli/mysqlconnection.cpp
Database/DBOperator.cpp
NotifReqHandler.cpp
../Message/Notification.cpp
posixMessageReceiveThread.cpp
notifyManager.cpp
notifytimeout.cpp
mainqueue.cpp
queuemessage.cpp
notifyidgen.cpp
../config/configreader.cpp
../msgqueue/posixmsgqueue.cpp
../msgqueue/ipcmsgqueue.cpp
../timer/timer.cpp
../timer/timerserver.cpp
../Message/Message.cpp
../Message/MsgReader.cpp
../Message/MsgWriter.cpp
../Message/itobytearray.cpp

../Crypt/tea.cpp
../Crypt/MD5.cpp

../thread/mutexwrap.cpp
../thread/conditionwrap.cpp
../thread/threadpool.cpp
../thread/mutexguard.cpp
../thread/threadwrap.cpp
)


include_directories(../)
link_directories(../thread/ ../Crypt/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_executable(NotifySrv_d ${src})

target_link_libraries(NotifySrv_d
mysqlclient
#threadpool
#libcrypt.a
event_core
event_pthreads
pthread rt)


