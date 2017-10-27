DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := test_aes 
INCLUDE := -I../../inc
LIB := -L../../lib -lapexbase

include ../../makefile.inc
$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)
