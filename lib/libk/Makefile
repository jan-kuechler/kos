## Makefile for libk.a ##

NAME=libk

BASE=../..
#BASE=/c/osdev/kos

CC=gcc
CC_INCL=-I$(BASE)/include -I$(BASE)/include/arch/i386 -I$(BASE)/kernel/include -Iinclude
CC_FLAGS=$(CC_INCL) -O3 -DLIBK -static -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs  -Wall -c

all: objs archive install clean

.PHONY: objs archive clean

objs:
	echo OBJS= *.c | sed -e 's/\.c/\.o/g' > obj.rules
	
-include obj.rules

archive: $(OBJS)
	ar rc $(NAME).a $(OBJS)
	
install:
	cp -f $(NAME).a $(BASE)/lib/
	
clean:
	rm -rf *.o
	rm -f libk.a
	rm -f obj.rules

%.o: %.c
	$(CC) $(CC_FLAGS) $<
