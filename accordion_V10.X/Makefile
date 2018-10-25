CC=gcc
CXX=g++ -O3 -Wall
RM=rm -f

CPPFLAGS= -pthread
LDFLAGS =
LDLIBS=-lpthread

SRCS=accordion_V10.x.cpp main_dispatcher.cpp INIReader.cpp aac.cpp base64.cpp common.cpp config.cpp handler_proc.cpp ini.cpp log.cpp loggermutex.cpp mpx.cpp msg_error.cpp parameters.cpp playlist.cpp tclientsocket.cpp url.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all:accordion_V9.x

accordion_V9.x: $(OBJS)
		$(CXX) $(LDFLAGS) -o accordion_V10.0 $(OBJS) $(LDLIBS)

depend: .depend
	depend:$(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

#include .depend