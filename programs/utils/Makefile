CC = i586-elf-gcc 
CFLAGS = -Wall -I../../share/include -L../../share/lib -Wl,-Tkos.ld

TESTCC = mingw32-gcc
TESTCFLAGS = -Wall -DTEST

SRCFILES = $(wildcard *.c)
TARGETS  = $(patsubst %.c, %, $(SRCFILES))

all: $(TARGETS)

install:
	@cp $(TARGETS) ../../initrd/bin

clean:
	@rm -f $(TARGETS)
	
%: %.c
	@echo Compiling $@
	@$(CC) $(CFLAGS) -o$@ $< -lc -lkos
