TARGET=log_agent
CONFIG=log_agent.conf

DIRS	=: ./
INCLUDE = -I../../base/util -I../../inc
LIBS = -L../../lib -L/usr/local/lib -lapexbase -lpthread -lev

include ../../makefile.inc

$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

install:
	cp -rf $(TARGET) /ecim/agent/log/
	cp -rf $(CONFIG) /ecim/agent/log/