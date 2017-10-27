DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := test_server test_client test_cmd
INCLUDE := -I../../inc -I ../../inc/hiredis -I ../../base/redis -I../../base/util
LIB := -L../../lib -lapexbase -lhiredis -lev
test_server_objs := test_server.o
test_client_objs := test_client.o
test_cmd_objs := test_cmd.o
include ../../makefile.inc

test_server:$(test_server_objs)
	$(XX) $(CFLAGS) -o $@ $(test_server_objs) $(LDFLAGS) $(LIB)
test_client:$(test_client_objs)
	$(XX) $(CFLAGS) -o $@ $(test_client_objs) $(LDFLAGS) $(LIB)
test_cmd:$(test_cmd_objs)
	$(XX) $(CFLAGS) -o $@ $(test_cmd_objs) $(LDFLAGS) $(LIB)

all:$(TARGET)

clean :
	rm -fr *.d *.o $(TARGET)
