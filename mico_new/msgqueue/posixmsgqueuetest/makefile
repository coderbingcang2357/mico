

.phoney: all

MSGHDR = ../posixmsgqueue.h 
MSGSRC = ../posixmsgqueue.cpp

OPT = -O2
all : server client
server : server.cpp $(MSGHDR) $(MSGSRC)
	g++ $(OPT)  server.cpp $(MSGSRC) -lrt -o server

client : client.cpp $(MSGHDR) $(MSGSRC)
	g++ $(OPT) client.cpp  $(MSGSRC) -lrt -o client
