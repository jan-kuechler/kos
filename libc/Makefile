## Makefile for libc.a ##

NAME=libc

CC=gcc
CC_INCL=-I../include -I../include/arch/i386 -I../kernel/include -Iinclude
CC_FLAGS=$(CC_INCL) -O3 -static -g -ffreestanding -nostdlib -nostartfiles -nodefaultlibs  -Wall -c

all: objs archive install clean

objs:
	echo OBJS= *.c | sed -e 's/\.c/\.o /g' > obj.rules
	
-include obj.rules

archive: $(OBJS)
	ar rc $(NAME).a $(OBJS)
	
install:
	cp -f $(NAME).a ../bin/
	
clean:
	rm -rf *.o
	rm -f obj.rules

%.o: %.c
	$(CC) $(CC_FLAGS) $<
