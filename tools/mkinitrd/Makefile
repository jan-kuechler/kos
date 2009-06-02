# use win32 or linux
HOST = win32

OUT_WIN32=mkid.exe
OUT_LINUX=mkid

#TEST=-DDUMMY
TEST=

CC=gcc
CFLAGS=-g -Wall 

all: $(HOST)

win32: main-win32.c initrd.c initrd.h
	$(CC) $(CFLAGS) $(TEST) -o$(OUT_WIN32) main-win32.c initrd.c
	
linux: main-linux.c initrd.c initrd.h
	$(CC) $(CFLAGS) $(TEST) -o$(OUT_LINUX) main-linux.c initrd.c

install:
	cp mkid.exe ../..
