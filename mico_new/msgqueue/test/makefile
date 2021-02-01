

Src = ../shmmsgqueue.cpp \
	  ../shmwrap.cpp \
	main.cpp

Hdr =  ../shmmsgqueue.h \
	   ../imem.h \
	   ../shmwrap.h

Lib = -lpthread -L../../thread -lthreadpool -lrt
Inc = -I../../
testmemqueue : $(Src) $(Hdr)
	g++ $(Inc) -g -o testmemqueue $(Src) $(Lib)
