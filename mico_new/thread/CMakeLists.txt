
set (Src conditionwrap.cpp
mutexwrap.cpp
threadpool.cpp
threadwrap.cpp
test.cpp
mutexguard.cpp
)

#Hdr += conditionwrap.h
#Hdr += mutexwrap.h
#Hdr += threadpool.h
#Hdr += threadwrap.h
#Hdr += mutexguard.h

#main: testpool libthreadpool.a

add_library(threadpool STATIC ${Src})
