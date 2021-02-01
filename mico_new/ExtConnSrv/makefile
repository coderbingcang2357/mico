src += main.cpp
src += ../util/util.cpp
src += ../util/logwriter.cpp
src += ../Message/Message.cpp
src += ../Message/MsgReader.cpp
src += ../Message/MsgWriter.cpp
src += Message/MsgInput.cpp
src += Message/MsgOutput.cpp
src += Business/BusinessManager.cpp
src += ../config/configreader.cpp
src += ../msgqueue/posixmsgqueue.cpp
src += ../msgqueue/ipcmsgqueue.cpp

CC = g++
FLAG = -Wall
OPTION = -lrt -L../thread -lthreadpool -lpthread -L../Crypt -lcrypt
IncludePath = -I../
ifeq ($(ver), debug)
	FLAG += -g
	MACRO = -D _DEBUG_VERSION
	EXEC_NAME = ../dbin/ExtConnSrv_d
else
	FLAG += -O2
	MACRO = -DNDEBUG
	EXEC_NAME = ../rbin/ExtConnSrv_d
endif
EXEC_PATH = ./$(EXEC_NAME)

$(EXEC_PATH) : $(src)
	@echo --------------------------$(EXEC_PATH)-----------------------------
	$(CC) $(IncludePath) $(MACRO) $(FLAG) $^ -o $(EXEC_PATH)  $(OPTION)
	@echo ----------------------------------------------------------------------


.PHONY:clean
clean:
	@echo clean ...
	-rm $(EXEC_PATH)
	@echo clean over ...
