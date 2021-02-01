
a.out : ../udpsession.h \
		../imessagerepeatchecker.h \
		../udpsession.cpp \
		../messagerepeatchecker.cpp \
		../messagerepeatchecker.h \
		../../thread/mutexwrap.cpp \
		../../thread/mutexguard.cpp main.cpp
	g++ -g -std=c++11 -I../../  ../udpsession.cpp \
	../messagerepeatchecker.cpp main.cpp \
	../../thread/mutexwrap.cpp \
	../../thread/mutexguard.cpp \
	-lpthread -o a.out 
