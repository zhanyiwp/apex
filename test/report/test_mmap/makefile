TARGET=test_mmap

INCLUDE = -I../../lib/wbl -I../../lib/nlb -I../../com -I../../../include
LIBS = -L../../lib/wbl -lwbl -L../../lib/nlb -lnlbapi -L../../../lib -lreportapi -Wl,-dn -Wl,-dy -lpthread

include ../../makefile.inc

install:
	cp -rf $(TARGET) /ecim/agent/monitor/

