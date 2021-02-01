src += main.cpp
src += ../util/util.cpp
src += ../util/logwriter.cpp
src += RelayReqHandler.cpp
src += ChannelManager.cpp
src += ./relaythread.cpp
src += ./mainqueue.cpp
src += ./relayinternalmessage.cpp
src += ../msgqueue/ipcmsgqueue.cpp
src += ../msgqueue/posixmsgqueue.cpp
src += ../Message/Message.cpp
src += ../Message/relaymessage.cpp
src += ../msgqueue/ipcmsgqueue.cpp
src += ../msgqueue/posixmsgqueue.cpp
src += ../Message/MsgReader.cpp
src += ../Message/MsgWriter.cpp
src += ../config/configreader.cpp
src += ./messagethread.cpp

CC = g++
FLAG = -Wall --std=c++0x
IncludePath = -I../
OPTION = -lmysqlclient -lm -lrt -L../Crypt -lcrypt -L../thread -lthreadpool -lpthread -lcurl 

ifeq ($(ver), debug)
	MACRO = -D _DEBUG_VERSION
	FLAG += -g
	EXEC_NAME = ../dbin/RelaySrv_d
else
	FLAG += -O2
	MACRO = -DNDEBUG
	EXEC_NAME = ../rbin/RelaySrv_d
endif

EXEC_PATH = ./$(EXEC_NAME)

$(EXEC_PATH):$(src)
	@echo --------------------------$(EXEC_PATH)-----------------------------
	@echo make ...
	$(CC) $(IncludePath) $(MACRO) $(FLAG) $^ -o $(EXEC_PATH) $(OPTION)
	@echo over ...
	@echo ----------------------------------------------------------------------

.PHONY:clean
clean:
	@echo clean ...
	rm $(EXEC_PATH)
	@echo clean over ...
