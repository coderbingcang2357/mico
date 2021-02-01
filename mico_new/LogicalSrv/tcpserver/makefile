
src = main.cpp \
	tcpserver.cpp \
	../../util/tcpdatapack.cpp

server : ${src}
	g++ -g -std=c++11 \
	-I../../ ${src} \
	-lpthread -levent_core -levent_pthreads \
	../../thread/libthreadpool.a -o server
