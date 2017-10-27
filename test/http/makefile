DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := test
INCLUDE :=  -I../../inc -I../../base/http -I../../base/util
LIB := -L../../lib -lapexbase  -lcurl -lev  -lssl 

include ../../makefile.inc
$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)
