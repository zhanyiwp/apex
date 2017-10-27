TARGET	:= svr_test
DIRS	:= $(shell find . -maxdepth 3 -type d)

SOURCE  := $(foreach dir, $(DIRS), $(wildcard ${dir}/*.c ${dir}/*.cpp ${dir}/*.cc))
OBJS    := $(patsubst %.c, %.o, $(patsubst %.cpp,%.o, $(patsubst %.cc, %.o, $(SOURCE))))
XX      := g++
LDFLAGS :=
DEFINES :=
INCLUDE := -I../../base/include/svr -I../../base/include/util
LIB		:= -L../../base/lib -lsvr -lev
CFLAGS  := -g -O0 -Wall -Wno-strict-aliasing -Wno-uninitialized $(DEFINES) $(INCLUDE) $(LIB)

.PHONY : all clean

all		:	$(TARGET)

-include *.d

.cpp.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.c.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.cc.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.S :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<

$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)

clean :
	rm -fr *.d *.o $(TARGET)
	rm -fr $(foreach dir, $(DIRS), $(wildcard ${dir}/*.o ${dir}/*.d ${dir}/*.pb.*))