SOURCE  := $(foreach dir, $(DIRS), $(wildcard ${dir}/*.c ${dir}/*.cpp ${dir}/*.cc ${dir}/*.S))
OBJS    := $(patsubst %.c, %.o, $(patsubst %.cpp,%.o, $(patsubst %.cc, %.o, $(patsubst %.S, %.o, $(SOURCE)))))
PB_FILE	:= $(foreach dir, $(DIRS), $(wildcard ${dir}/*.proto))
PROTOC 	:= $(shell find ~ -name protoc | grep "apex/script/protoc")
XX      := g++
LDFLAGS :=
DEFINES :=
CFLAGS  := -g -O0 -Wall -Wno-strict-aliasing -Wno-uninitialized $(DEFINES) $(INCLUDE) $(LIB) -DOPTION_REPORT_MMAP_FILEPATH=\"/gaz/agent/report/report.mmap\" -DOPTION_REPORT_MMAP_ELEMENTSIZE=32 -DOPTION_REPORT_MMAP_ELEMENTCOUNT=20000000  -DOPTION_LOG_MMAP_FILEPATH=\"/gaz/agent/log/log.mmap\" -DOPTION_LOG_MMAP_ELEMENTSIZE=32 -DOPTION_LOG_MMAP_ELEMENTCOUNT=20000000

.PHONY : all clean

all     : $(TARGET)

clean :
	rm -fr *.d *.o $(TARGET)
	rm -fr $(foreach dir, $(DIRS), $(wildcard ${dir}/*.o ${dir}/*.d ${dir}/*.pb.*))

-include *.d

.cpp.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.c.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.cc.o :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<
.S :
	$(XX) $(CFLAGS) -MMD -MF $*.d -c -o $@ $<

proto:
	$(shell chmod +x $(PROTOC))
	@echo $(PB_FILE)
	$(shell $(PROTOC) $(PB_FILE) --cpp_out=.)
	$(shell $(SCRIPT))
