CC = kgcc
CFLAGS = -Wall 

TESTCC = mingw32-gcc
TESTCFLAGS = -Wall -DTEST

SRCFILES = $(shell echo "*.c")

all: radau tests

radau: $(SRCFILES)
	@$(CC) $(CFLAGS) -oradau $(SRCFILES) -lk

install:
	@cp radau ../../initrd/bin

# Maintain this list manually, not every file has a test
tests:

clean:
	rm -f radau
	rm -f *.exe
