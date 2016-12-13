CROSS_COMPILE =
AS            = $(CROSS_COMPILE)as
LD            = $(CROSS_COMPILE)ld
CC            = $(CROSS_COMPILE)gcc
CPP           = $(CC) -E
AR            = $(CROSS_COMPILE)ar
NM            = $(CROSS_COMPILE)nm
STRIP         = $(CROSS_COMPILE)strip
OBJCOPY       = $(CROSS_COMPILE)objcopy
OBJDUMP       = $(CROSS_COMPILE)objdump

export AS LD CC CPP AR NM
export STRIP OBJCOPY OBJDUMP

CFLAGS := -Wall -Werror -O2 -g
CFLAGS += -I $(shell pwd)/include
LDFLAGS := -lm -ljpeg -lvga -lvgagl

TOPDIR := $(shell pwd)

TARGET := video2lcd

obj-y += main.o
obj-y += display/
obj-y += video/
obj-y += convert/

all :
	echo "/* This file is auto generate */" > include/compile_time.h
	echo "#define COMPILE_DATE \"$(shell date +%Y-%m-%d\ %H:%M)\" " >> include/compile_time.h
	make -C ./ -f $(TOPDIR)/Makefile.build
	$(CC) $(LDFLAGS) -o $(TARGET) built-in.o
	ctags -R

clean:
	rm -f $(shell find -name "*.o")
	rm -f $(TARGET) tags include/compile_time.h

distclean:
	rm -f $(shell find -name "*.o")
	rm -f $(shell find -name "*.d")
	rm -f $(TARGET) tags include/compile_time.h

export CFLAGS LDFLAGS
export TOPDIR
