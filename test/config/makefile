DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := config_test
INCLUDE := -I../../inc
LIB := -L../../lib -lapexbase

include ../../makefile.inc
$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)