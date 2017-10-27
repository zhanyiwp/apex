DIRS	:= $(shell find . -maxdepth 3 -type d ! -path "./mysqlconnector*")
TARGET  := ../lib/libapexbase.a
INCLUDE := -I./util -I./net -I./nbmysql -I../inc/ -I../inc/cryptopp -I../inc/mariadb/ -I../inc/hiredis/ -I../inc/libev -I/usr/local/include/zookeeper -I../thirdparty/jsoncpp-src-0.5.0/include
LIB := -L../lib -L../thirdparty/zookeeper-3.4.9/src/c -L../thirdparty/jsoncpp-src-0.5.0/libs/linux-gcc-4.4.7 -lpb64 -lcryptopp -lzookeeper_mt -ljson_linux-gcc-4.4.7_libmt
include ../makefile.inc
$(TARGET) : $(OBJS)
	ar -rs $@ $^
