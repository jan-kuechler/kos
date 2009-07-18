CC = kgcc
CFLAGS = -Wall 

TESTCC = mingw32-gcc
TESTCFLAGS = -Wall -DTEST

SRCFILES = $(shell echo "*.c")

all: sh tests

sh: $(SRCFILES)
	@$(CC) $(CFLAGS) -osh $(SRCFILES) -lk

install:
	@cp sh ../../initrd/bin

# Maintain this list manually, not every file has a test
tests: util.exe

util.exe: util.c util.h
	@$(TESTCC) $(TESTCFLAGS) -outil util.c

clean:
	rm -f sh
	rm -f *.exe
