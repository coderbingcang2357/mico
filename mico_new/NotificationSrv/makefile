src += main.cpp
src+= BusinessManager.cpp
src+= ../util/util.cpp
src+= ../util/logwriter.cpp
src+= ../mysqlcli/mysqlconnection.cpp
src+= Database/DBOperator.cpp
src+= NotifReqHandler.cpp
src+= ../Message/Notification.cpp
src+= posixMessageReceiveThread.cpp
src+= notifyManager.cpp
src+= notifytimeout.cpp
src+= mainqueue.cpp
src+= queuemessage.cpp
src+= ../config/configreader.cpp
src+= ../msgqueue/posixmsgqueue.cpp
src+= ../msgqueue/ipcmsgqueue.cpp
src+= ../timer/timer.cpp
src+= ../timer/timerserver.cpp
src+= ../Message/Message.cpp
src+= ../Message/MsgReader.cpp
src+= ../Message/MsgWriter.cpp
src+= ../Message/itobytearray.cpp

CC = g++
FLAG = -Wall -std=c++0x
IncludePath = -I../
OPTION = -lmysqlclient -lm -lrt -L../thread -lthreadpool -lpthread  -L../Crypt -lcrypt
ifeq ($(ver), debug)
	MACRO = -D _DEBUG_VERSION
	FLAG += -g
	EXEC_NAME = ../dbin/NotifySrv_d
else
	FLAG += -O2
	MACRO = -DNDEBUG
	EXEC_NAME = ../rbin/NotifySrv_d
endif

EXEC_PATH = ./$(EXEC_NAME)

$(EXEC_PATH):$(src)
	@echo --------------------------$(EXEC_PATH)--------------------------------
	$(CC) $(IncludePath) $(MACRO) $(FLAG) $^ -o $(EXEC_PATH)  $(OPTION)
	@echo ----------------------------------------------------------------------

.PHONY:clean
clean:
	@echo clean ...
	-rm $(EXEC_PATH)
	@echo clean over
