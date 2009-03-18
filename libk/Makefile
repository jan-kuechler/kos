## Makefile for libkos.a ##

CC=gcc
CC_INCL=-I../include -I../include/arch/i386 -Iinclude
CC_FLAGS=$(CC_INCL) -O3 -static -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs  -Wall -c

OBJS=etc.o file.o io.o ipc.o helper.o

all: archive install clean

archive: $(OBJS)
	ar rc libk.a $(OBJS)
	
install:
	cp -f libk.a ../bin/
	
clean:
	rm -rf *.o

%.o: %.c
	$(CC) $(CC_FLAGS) $<
