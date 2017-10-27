TARGET=test_log

DIRS	=: ./
INCLUDE = -I../../base/util -I../../inc
LIBS = -L../../lib -L/usr/local/lib -lapexbase -lpthread -lev

include ../../makefile.inc

$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

install:
	cp -rf $(TARGET) /ecim/agent/log/