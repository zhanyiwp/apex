DIRS	:= $(shell find . -maxdepth 3 -type d)
TARGET := test
INCLUDE := -I../../inc/mariadb -I../../inc -I../../base/mysql -I../../base/util
LIB := -L../../lib -lapexbase -lmariadbclient -lev  -lssl

include ../../makefile.inc
$(TARGET) : $(OBJS)
	$(XX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIB)
