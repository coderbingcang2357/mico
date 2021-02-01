cmake_minimum_required(VERSION 2.8.1)

set(src main.cpp
../util/util.cpp
../util/logwriter.cpp
../Message/Message.cpp
../Message/MsgReader.cpp
../Message/MsgWriter.cpp
Message/MsgInput.cpp
Message/MsgOutput.cpp
Business/BusinessManager.cpp
../config/configreader.cpp
../msgqueue/posixmsgqueue.cpp
../msgqueue/ipcmsgqueue.cpp
)

include_directories(../)
link_directories(../thread/ ../Crypt/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
add_executable(ExtConnSrv_d ${src})

target_link_libraries(ExtConnSrv_d threadpool libcrypt.a pthread rt)

