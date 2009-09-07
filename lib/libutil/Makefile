## Makefile for libc.a ##

NAME = libutil.a

AR = i586-elf-ar

INCLUDE_DIRS = -Ishare/include -Ishare/include/compiler/$(COMPILER)  \
               -Ilib/libc/includes -Ilib/libc/internals \
               -Ilib/libutil/include \
               -Ilib/libkos/include

CC = i586-elf-gcc
CFLAGS = -Wall -O2 -static -g $(INCLUDE_DIRS)

all: archive install clean

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

archive: $(OBJS)
	@rm -f $(NAME)
	@$(AR) rc $(NAME) $(OBJS)
	
install:
	@cp -f $(NAME) ..
	
clean:
	@rm -f $(OBJS)

%.o: %.c
	$(CC) $(CC_FLAGS) -o$@ -c $<
