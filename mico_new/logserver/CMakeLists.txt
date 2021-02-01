cmake_minimum_required(VERSION 2.8.1)

set(src main.cpp
./logread.cpp
./logserver.cpp
)

include_directories(../)
link_directories(../thread/ ../Crypt/ ${LIBRARY_OUTPUT_PATH})
#add_compile_options(-Wall)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")
add_executable(Logserver ${src})

target_link_libraries(Logserver pthread rt)

