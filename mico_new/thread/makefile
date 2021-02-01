

Src += conditionwrap.cpp
Src += mutexwrap.cpp
Src += threadpool.cpp
Src += threadwrap.cpp
Src += test.cpp
Src += mutexguard.cpp


Hdr += conditionwrap.h
Hdr += mutexwrap.h
Hdr += threadpool.h
Hdr += threadwrap.h
Hdr += mutexguard.h

.PHONY : main
main: testpool libthreadpool.a

testpool : $(Src) $(Hdr)
	g++ -std=c++11 -g -o testpool $(Src) -lpthread

libthreadpool.a : $(Src) $(Hdr)
	gcc -std=c++11 -c -g --std=c++0x -DNDEBUG $(Src)
	ar crs libthreadpool.a conditionwrap.o mutexwrap.o threadpool.o threadwrap.o mutexguard.o

clean:
	-rm conditionwrap.o mutexwrap.o threadpool.o threadwrap.o mutexguard.o libthreadpool.a
