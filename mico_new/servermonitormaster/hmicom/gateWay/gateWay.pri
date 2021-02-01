HEADERS +=    $$PWD/udpL1L2.h \
	      $$PWD/udpsocket.h 
		
SOURCES +=    $$PWD/udpL1L2.cpp \
		$$PWD/udpsocket.cpp 

android {
}else{
LIBS += -lwsock32
}
		
