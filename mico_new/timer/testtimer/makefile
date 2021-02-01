

SOURCES = main.cpp \
		  ../timer.cpp \
		  ../timerserver.cpp \
		../../thread/threadwrap.cpp \
		  ../../thread/mutexwrap.cpp \
		  ../../thread/conditionwrap.cpp \

HEADERS = ../../thread/threadwrap.h  \
		  ../../thread/mutexwrap.h \
		../timer.h \
		  ../timerserver.h \
		  ../../thread/conditionwrap.h

Inc = -I../../
Lib = -L../../thread -lpthread

all : testtimer clocktimer wheeltimer

testtimer : $(SOURCES) $(HEADERS)
	g++ -O2  -std=c++11 $(Inc)  $(SOURCES) -o testtimer $(Lib)

clocktimer : ./testclocktimer.cpp ../clocktimer.cpp ../clocktimer.h
	g++ -O2 -std=c++11 -D__TEST__ ./testclocktimer.cpp ../clocktimer.cpp -o clocktimer

wheeltimer : ../wheeltimer.cpp ../wheeltimer.h ./testwheel.cpp
	g++ -g -std=c++11 ../wheeltimer.cpp ./testwheel.cpp -o wheeltimer

