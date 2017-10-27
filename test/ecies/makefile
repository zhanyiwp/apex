DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := test_ecies
INCLUDE := -I../../inc -I/usr/local/include/cryptopp
LIB := -L../../lib -lapexbase -L/usr/local/lib -lcryptopp

include ../../makefile.inc
$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)
