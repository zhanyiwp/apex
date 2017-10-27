TARGET=$(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4)
TARGET1=balance_client_test
TARGET2=balance_server_test
TARGET3=master_client_test
TARGET4=master_server_test
OBJS1=balance_client_test.o
OBJS2=balance_server_test.o
OBJS3=master_client_test.o
OBJS4=master_server_test.o
CONFIG=balance_client_test.conf balance_server_test.conf master_client_test.conf master_server_test.conf

DIRS	=: ./
INCLUDE = -I../../base/util -I../../inc
LIBS = -L../../lib -L/usr/local/lib -L../../thirdparty/jsoncpp-src-0.5.0/libs/linux-gcc-4.4.7 -lapexbase -lpthread -lev -lzookeeper_mt -ljson_linux-gcc-4.4.7_libmt

include ../../makefile.inc

$(TARGET1) : $(OBJS1)
	$(XX) $(CFLAGS) -o $@ $(OBJS1) $(LDFLAGS) $(LIBS)
$(TARGET2) : $(OBJS2)
	$(XX) $(CFLAGS) -o $@ $(OBJS2) $(LDFLAGS) $(LIBS)	
$(TARGET3) : $(OBJS3)
	$(XX) $(CFLAGS) -o $@ $(OBJS3) $(LDFLAGS) $(LIBS)	
$(TARGET4) : $(OBJS4)
	$(XX) $(CFLAGS) -o $@ $(OBJS4) $(LDFLAGS) $(LIBS)	

install:
	cp -rf $(TARGET) /ecim/roof/name/
	cp -rf $(CONFIG) /ecim/roof/name/	