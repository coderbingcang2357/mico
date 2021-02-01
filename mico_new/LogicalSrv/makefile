src += main.cpp

src += Business/Business.cpp
src += claimdevice.cpp
src += claimmanager.cpp
src += ../util/util.cpp
src += ../util/logwriter.cpp
src += ../protocoldef/protocol.cpp
src += ../Message/Message.cpp
src += ../Message/MsgReader.cpp
src += ../Message/MsgWriter.cpp
src += ../Message/Notification.cpp
src += ../Message/relaymessage.cpp
src += ../Message/itobytearray.cpp
src += onlineinfo/mainthreadmsgqueue.cpp
src += onlineinfo/onlineinfo.cpp
src += onlineinfo/userdata.cpp
src += onlineinfo/devicedata.cpp
src += onlineinfo/mainthreadmsgqueue.cpp
src += onlineinfo/deviceOfflineCommand.cpp
src += onlineinfo/rediscache.cpp
src += onlineinfo/userofflineCommand.cpp
src += onlineinfo/heartbeattimer.cpp
src += onlineinfo/shmcache.cpp
src += onlineinfo/devicetimeoutcommand.cpp
src += onlineinfo/usertimeoutcommand.cpp
src += ../msgqueue/shmwrap.cpp
src += ../msgqueue/memoryhash.cpp
src += ../config/configreader.cpp
src += ../timer/timer.cpp
src += ../timer/timerserver.cpp
src += ../msgqueue/posixmsgqueue.cpp
src += ../msgqueue/ipcmsgqueue.cpp
src += ../mysqlcli/mysqlconnpool.cpp
src += ../mysqlcli/mysqlconnection.cpp
src += ./useroperator.cpp
src += ./sessionkeygen.cpp
src += ./deviceregister.cpp
src += ./devicelogin.cpp 
src += ./userpasswordrecover.cpp 
src += ./mailsender.cpp 
src += ./usermodifymail.cpp 
src += ./messageencrypt.cpp 
src += ./usermessageprocess.cpp 
src += tcpserver/tcpserver.cpp 
src += ./forwardmessage.cpp 
src += ./messageforward.cpp 
src += ./messagesender.cpp 
src += ../util/tcpdatapack.cpp 
src += loginprocessor.cpp 
src += searchuser.cpp 
src += scenemessageprocess.cpp
src += messagereceiver.cpp
src += newmessagecommand.cpp
src += feedback.cpp
src += ../serverinfo/serverinfo.cpp
src += ./messagefromnotifysrv.cpp
src += ./deletenotifymessage.cpp
src += ./onlineinfo/redisconnectionpool.cpp
src += ./fileuploadredis.cpp
src += ./gennotifyuseronlinestatus.cpp
src += ./transferdevicecluster.cpp
src += ./getserverlist.cpp


CC = g++
FLAG = -Wall -std=c++0x
#FLAG = -Wall -std=c++11

# for tea.h and md5.h
IncludePath = -I../

OPTION = -lmysqlclient -lm -lrt -L../Crypt -lcrypt -L../thread -lthreadpool -lpthread -lcurl -levent_core -levent_pthreads -lamqpcpp
OPTION += ../hiredis/libhiredis.a

ifeq ($(ver), debug)
	MACRO = -D _DEBUG_VERSION
	FLAG += -g
	EXEC_NAME = ../dbin/LogicalSrv_d
else
	FLAG += -O2
	MACRO = -DNDEBUG
	EXEC_NAME = ../rbin/LogicalSrv_d
endif
EXEC_PATH = ./$(EXEC_NAME)

$(EXEC_PATH):$(src)
	@echo --------------------------$(EXEC_PATH)--------------------------------
	$(CC) $(IncludePath) $(MACRO) $(FLAG) $^ -o $(EXEC_PATH) $(OPTION)
	@echo ----------------------------------------------------------------------

.PHONY:clean
clean:
	@echo clean ...
	-rm $(EXEC_PATH)
	@echo clean over
