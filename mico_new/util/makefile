
.phony:all
all: m testptr testtcppack 

m : util.cpp
	g++ -c util.cpp -o m.o

testptr : sharedptr.cpp sharedptr.h
	g++ -g -o testptr sharedptr.cpp

testtcppack : ./tcpdatapack.h tcpdatapack.cpp testtcppack.cpp
	g++ -g -o testtcppack tcpdatapack.cpp ./testtcppack.cpp
