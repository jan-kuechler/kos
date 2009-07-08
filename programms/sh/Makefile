CC = kgcc
CFLAGS = -Wall -lk

SRCFILES = $(shell echo "*.c")

all: sh

sh: $(SRCFILES)
	@$(CC) $(CFLAGS) -osh $(SRCFILES)

install:
	@cp sh ../../initrd/bin
